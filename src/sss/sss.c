/**
 * @file sss.c
 *
 * @brief Shamir's Secret Sharing (SSS) implementation over GF(256).
 *
 * @author Julien F.
 * @date 2026-07-12
 *
 * @details Uses the AES irreducible polynomial: x^8 + x^4 + x^3 + x + 1  (0x11B).
 *          GF(256) addition is XOR; multiplication and division use precomputed
 *          log/exp tables for efficiency on the ESP32.
 *
 *          Randomness is sourced from Zephyr's sys_rand_get().
 */

#include "sss.h"

#include <zephyr/random/random.h>

#include <errno.h>
#include <string.h>

// ===========================================================================
// Structure and variables definition
// ===========================================================================
/* ---------------------------------------------------------------------------
 * GF(256) arithmetic  (irreducible polynomial 0x11B)
 * ------------------------------------------------------------------------- */
/*
 * exp[i] = g^i,  log[i] = discrete log base g.
 *
 * We use g = 0x03 (x+1) as the generator — it IS a primitive element
 * for the AES polynomial x^8 + x^4 + x^3 + x + 1 (0x11B), generating
 * all SSS_FINITE_GALOIS_FIELD-1 non-zero elements.  (g = 0x02, i.e. simple left-shift, has
 * order 51 only and would leave most log[] entries uninitialised.)
 */
static uint8_t gf_exp[2 * SSS_FINITE_GALOIS_FIELD]; /* doubled to avoid mod-255 in multiplication */
static uint8_t gf_log[SSS_FINITE_GALOIS_FIELD];

// ===========================================================================
// Static function declarations
// ===========================================================================
/**
 * @brief Lagrange basis polynomial L_i(0) for the given x-coordinates.
 *
 * L_i(0) = ∏_{j≠i}  x_j / (x_j - x_i)    (all operations in GF(256))
 *
 * The result is a scalar multiplier for share i's y-value.
 *
 * @param[in] x_vals    Array of x-coordinates.
 * @param[in] k         Number of x-coordinates.
 * @param[in] i         Index of the Lagrange basis polynomial.
 *
 * @return L_i(0) in GF(256).
 */
static uint8_t lagrange_basis_0(const uint8_t *x_vals, unsigned int k, unsigned int i);

/**
 * @brief Initialize GF(256) exp/log tables (lazy).
 *
 * This function is called once before any SSS operations.
 */
static void gf_init_tables(void);

/**
 * @brief GF(256) arithmetic operation: addition.
 *
 *
 * @param[in] a  First operand.
 * @param[in] b  Second operand.
 *
 * @return Result of a + b in GF(256).
 */
static inline uint8_t gf_add(uint8_t a, uint8_t b);

/**
 * @brief GF(256) arithmetic operation: subtraction.
 *
 * In GF(256), addition and subtraction are the same (XOR).
 *
 * @param[in] a  First operand.
 * @param[in] b  Second operand.
 *
 * @return Result of a - b in GF(256).
 */
static inline uint8_t gf_sub(uint8_t a, uint8_t b);

/**
 * @brief GF(256) arithmetic operation: multiplication.
 *
 * Uses precomputed exp/log tables for efficiency.
 *
 * @param[in] a  First operand.
 * @param[in] b  Second operand.
 *
 * @return Result of a * b in GF(256).
 */
static uint8_t gf_mul(uint8_t a, uint8_t b);

/**
 * @brief GF(256) arithmetic operation: division.
 *
 * Uses precomputed exp/log tables for efficiency.
 *
 * @param[in] a  Numerator.
 * @param[in] b  Denominator (must be non-zero).
 *
 * @return Result of a / b in GF(256).
 */
static uint8_t gf_div(uint8_t a, uint8_t b);

/**
 * @brief Evaluate a polynomial at a given x in GF(256).
 *
 * Uses Horner's method for efficient evaluation.
 *
 * @param[in] coeff  Coefficients of the polynomial (degree k-1).
 * @param[in] k      Number of coefficients (degree + 1).
 * @param[in] x      Point at which to evaluate the polynomial.
 *
 * @return Polynomial value at x in GF(256).
 */
static uint8_t gf_poly_eval(const uint8_t *coeff, unsigned int k, uint8_t x);
// ===========================================================================
// Public function definition
// ===========================================================================
int sss_split(const uint8_t *secret, size_t secret_len, unsigned int n, unsigned int k, struct sss_share *shares_out)
{
    int ret = -1;

    if (secret == NULL || shares_out == NULL || secret_len == 0 || secret_len > SSS_MAX_SECRET_LEN || n < k || k < 2 ||
        n > (SSS_FINITE_GALOIS_FIELD - 1))
    {
        ret = -EINVAL;
        goto exit;
    }

    gf_init_tables();

    /* Random coefficients buffer (k-1 random bytes per secret byte). */
    uint8_t coeff[SSS_MAX_SECRET_LEN]; /* reused per byte; k-1 random + 1 secret */

    for (size_t byte_idx = 0; byte_idx < secret_len; byte_idx++)
    {
        /* Generate k-1 random coefficients */
        size_t rand_needed = k - 1;
        sys_rand_get(&coeff[1], rand_needed); /* coeff[1..k-1] = random */
        coeff[0] = secret[byte_idx]; /* coeff[0] = secret byte */

        /* Evaluate polynomial at x = 1..n */
        for (unsigned int xi = 0; xi < n; xi++)
        {
            uint8_t x = (uint8_t)(xi + 1);
            uint8_t y = gf_poly_eval(coeff, k, x);

            if (byte_idx == 0)
            {
                shares_out[xi].x = x;
                shares_out[xi].len = secret_len;
            }
            shares_out[xi].data[byte_idx] = y;
        }
    }

    ret = 0; /* success */

exit:
    return (ret);
}

int sss_combine(const struct sss_share *shares, unsigned int k, uint8_t *secret_out, size_t secret_len)
{
    int ret = -1;

    if (shares == NULL || secret_out == NULL || secret_len == 0 || secret_len > SSS_MAX_SECRET_LEN || k < 2)
    {
        ret = -EINVAL;
        goto exit;
    }

    gf_init_tables();

    /* Collect x-values */
    uint8_t x_vals[SSS_FINITE_GALOIS_FIELD];
    for (unsigned int i = 0; i < k; i++)
    {
        x_vals[i] = shares[i].x;
    }

    /* Precompute Lagrange basis values at x=0 */
    uint8_t basis[SSS_FINITE_GALOIS_FIELD];
    for (unsigned int i = 0; i < k; i++)
    {
        basis[i] = lagrange_basis_0(x_vals, k, i);
    }

    /* Reconstruct each byte */
    for (size_t byte_idx = 0; byte_idx < secret_len; byte_idx++)
    {
        uint8_t acc = 0;
        for (unsigned int i = 0; i < k; i++)
        {
            acc = gf_add(acc, gf_mul(basis[i], shares[i].data[byte_idx]));
        }
        secret_out[byte_idx] = acc;
    }

    ret = 0; /* success */

exit:
    return (ret);
}

// ===========================================================================
// Static function definition
// ===========================================================================
static uint8_t lagrange_basis_0(const uint8_t *x_vals, unsigned int k, unsigned int i)
{
    uint8_t num = 1;
    uint8_t den = 1;

    for (unsigned int j = 0; j < k; j++)
    {
        if (j == i)
        {
            continue;
        }
        num = gf_mul(num, x_vals[j]); /* num *= x_j */
        den = gf_mul(den, gf_sub(x_vals[j], x_vals[i])); /* den *= (x_j - x_i) */
    }

    return gf_div(num, den); /* L_i(0) */
}

static void gf_init_tables(void)
{
    static int gf_tables_ready;

    if (gf_tables_ready)
    {
        return;
    }

    unsigned int i;
    uint16_t x = 1; /* g^0 = 1 */

    for (i = 0; i < SSS_FINITE_GALOIS_FIELD - 1; i++)
    {
        gf_exp[i] = (uint8_t)x;
        gf_exp[i + SSS_FINITE_GALOIS_FIELD - 1] = (uint8_t)x; /* duplicate for wrap-free lookup */
        gf_log[(uint8_t)x] = (uint8_t)i;

        /* multiply x by g = 0x03 = x+1  →  x * (x+1) = x*2 + x */
        x <<= 1; /* x * 2 */
        if (x & 0x100)
        {
            x ^= 0x11B; /* reduce by irreducible poly */
        }
        x ^= (uint16_t)gf_exp[i]; /* + original value = multiply by 0x03 */
    }
    gf_log[0] = 0; /* log(0) is undefined; set to 0 for safety */
    gf_tables_ready = 1;
}

static inline uint8_t gf_add(uint8_t a, uint8_t b)
{
    return a ^ b;
}

static inline uint8_t gf_sub(uint8_t a, uint8_t b)
{
    return a ^ b; /* same as add in GF(2^n) */
}

static uint8_t gf_mul(uint8_t a, uint8_t b)
{
    if (a == 0 || b == 0)
    {
        return 0;
    }
    return gf_exp[gf_log[a] + gf_log[b]];
}

static uint8_t gf_div(uint8_t a, uint8_t b)
{
    if (b == 0)
    {
        return 0; /* division by zero → return 0 (should not happen) */
    }
    if (a == 0)
    {
        return 0;
    }
    /* a / b = g^(log(a) - log(b)) = g^(log(a) + 255 - log(b)) */
    return gf_exp[(gf_log[a] + SSS_FINITE_GALOIS_FIELD - 1 - gf_log[b])];
}

static uint8_t gf_poly_eval(const uint8_t *coeff, unsigned int k, uint8_t x)
{
    uint8_t ret = 0;

    if (k == 0)
    {
        goto exit;
    }

    /* Horner: result = coeff[k-1]; for i=k-2..0: result = result*x + coeff[i] */
    ret = coeff[k - 1];

    for (int i = (int)k - 2; i >= 0; i--)
    {
        ret = gf_add(gf_mul(ret, x), coeff[i]);
    }

exit:
    return (ret);
}