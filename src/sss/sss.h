/**
 * @file sss.h
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


#ifndef SSS_H
#define SSS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

// ===========================================================================
// Typedef and structure definition
// ===========================================================================
#define SSS_FINITE_GALOIS_FIELD (256) /* GF(2^8) */

/** Maximum supported secret length in bytes (arbitrary; tune as needed). */
#define SSS_MAX_SECRET_LEN (256)

/** Default number of shares to generate. */
#define SSS_N (5)

/** Default threshold (minimum shares required to reconstruct). */
#define SSS_K (3)

/**
 * @brief One share: the x-coordinate and the share data bytes.
 */
struct sss_share
{
    uint8_t x; /* x-coordinate (1..N) */
    uint8_t data[SSS_MAX_SECRET_LEN]; /* share bytes (same length as secret) */
    size_t len; /* actual byte length of share data */
};

// ===========================================================================
// Public function declaration
// ===========================================================================
/**
 * @brief Split a secret into N shares (K required to reconstruct).
 *
 * @param[in] secret        Input secret bytes.
 * @param[in] secret_len    Length of secret in bytes (≤ SSS_MAX_SECRET_LEN).
 * @param[in] n             Number of shares to produce (e.g. 5).
 * @param[in] k             Threshold (e.g. 3).
 * @param[out] shares_out    Output array of n shares (caller allocates n elements).
 *
 * @return 0 on success, negative on error.
 */
int sss_split(const uint8_t *secret, size_t secret_len, unsigned int n, unsigned int k, struct sss_share *shares_out);

/**
 * @brief Reconstruct a secret from K shares using Lagrange interpolation.
 *
 * @param[in] shares        Array of exactly k shares.
 * @param[in] k             Number of shares provided (threshold).
 * @param[out] secret_out    Output buffer for reconstructed secret.
 * @param[in] secret_len    Length of the secret (must match original secret_len).
 *
 * @return 0 on success, negative on error.
 */
int sss_combine(const struct sss_share *shares, unsigned int k, uint8_t *secret_out, size_t secret_len);

#ifdef __cplusplus
}
#endif

#endif /* SSS_H */
