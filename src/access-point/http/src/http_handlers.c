/**
 * @file http_handlers.c
 *
 * @brief HTTP request handlers for the captive portal.
 *
 * @author Julien F.
 * @date 2026-07-12
 *
 * @details This module implements HTTP request handlers for the captive portal.
 *          It serves the appropriate HTML pages based on the request path.
 */

#include "http_handlers.h"

#include <zephyr/logging/log.h>

#include "http_types.h"
#include "page_captive.h"
#include "sss.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ===========================================================================
// Zephyr logging module registration
// ===========================================================================
LOG_MODULE_REGISTER(handlers, LOG_LEVEL_DBG);

// ===========================================================================
// Static function declarations
// ===========================================================================

/**
 * @brief URL-decode a string in-place.
 *
 * Decodes %XX sequences and converts '+' to space.
 *
 * @param dst[out] Destination buffer for the decoded string.
 * @param src[in] Source string to decode.
 * @param dst_size[in] Size of the destination buffer.
 *
 * @return None. The decoded string is null-terminated.
 */
static void url_decode(char *dst, const char *src, size_t dst_size);

/**
 * @brief Encrypt a secret using Shamir's Secret Sharing (SSS).
 *
 * Splits the secret into shares and returns them.
 *
 * @param secret[in] The secret to encrypt.
 * @param shares_out[out] Array to hold the generated shares.
 * @param shares_count[out] Number of shares generated.
 *
 * @return 0 on success, negative on error.
 */
static int handler_shamir_split(const char *secret, struct sss_share *shares_out, size_t *shares_count);

/**
 * @brief Parse entry shares from query parameters.
 *
 * Extracts the shares from the query parameters and populates the out_shares array.
 *
 * @param d_dec[in] Comma-separated hex strings of share data.
 * @param x_dec[in] Comma-separated x-coordinates of shares.
 * @param out_shares[out] Array to hold the parsed shares.
 * @param required_shares[in] Number of shares expected.
 *
 * @return 0 on success, negative on error.
 */
static int handler_parse_entry_share(const char *d_dec,
                                     const char *x_dec,
                                     struct sss_share *out_shares,
                                     uint8_t required_shares);

/**
 * @brief Generate a JSON response containing the shares.
 *
 * Formats the shares into a JSON string and returns it.
 *
 * @param shares[in] Array of shares to include in the response.
 *
 * @return Pointer to the complete HTTP response string (static, do not free).
 */
static const char *handler_shares_json_response(const struct sss_share *shares);

/**
 * @brief Generate a JSON response containing the reconstructed secret.
 *
 * Formats the secret into a JSON string and returns it.
 *
 * @param secret[in] The reconstructed secret to include in the response.
 *
 * @return Pointer to the complete HTTP response string (static, do not free).
 */
static const char *handler_combine_json_response(const char *secret);

/**
 * @brief Parse a hex string (e.g. "5a021ac0") into raw bytes.
 *
 * @param hex[in]    Null-terminated hex string (even length).
 * @param out[out]   Destination buffer (at least len/2 bytes).
 * @param out_max[in] Maximum bytes to write.
 *
 * @return Number of bytes written, or -1 on error.
 */
static int hex_to_bytes(const char *hex, uint8_t *out, size_t out_max);
/**
 * @brief Extract the value of a query parameter from a query string.
 *
 * @param query[in]  The full query string.
 * @param param[in]  Parameter name (without '=').
 * @param out[out]   Buffer for the value.
 * @param out_size[in] Size of the output buffer.
 *
 * @return 0 on success, -1 if not found.
 */
static int get_query_param(const char *query, const char *param, char *out, size_t out_size);

/**
 * @brief Count the number of comma-separated tokens in a string.
 *
 * @param s[in] Comma-separated string.
 *
 * @return Number of tokens (0 if empty or NULL).
 */
static int count_tokens(const char *s);

/**
 * @brief Extract the n-th comma-separated token from a string.
 *
 * @param s[in]     Comma-separated string.
 * @param index[in] 0-based token index.
 * @param out[out]  Buffer for the token.
 * @param out_size[in] Size of the output buffer.
 *
 * @return 0 on success, -1 if not found.
 */
static int get_token(const char *s, int index, char *out, size_t out_size);
// ===========================================================================
// Public functions definition
// ===========================================================================
const char *handler_root(const struct http_request *req)
{
    (void)req;
    return PAGE_CAPTIVE;
}

const char *handler_split(const struct http_request *req)
{
    (void)req;
    return PAGE_SPLIT;
}

const char *handler_unsplit(const struct http_request *req)
{
    (void)req;
    return PAGE_UNSPLIT;
}

const char *handler_captive_portal(const struct http_request *req)
{
    (void)req;
    /* Any unrecognised request = captive portal page.
     * This covers Android (/generate_204),
     * Apple (/hotspot-detect.html), Windows (/ncsi.txt), etc.
     * Receiving a 200 + HTML instead of the expected
     * response is what triggers the captive portal popup. */
    return PAGE_CAPTIVE;
}

// ===========================================================================
// Public functions
// ===========================================================================
const char *handler_divide(const struct http_request *req)
{
    const char *ret = http_responses_list[HTTP_RESPONSE_BAD_REQUEST];

    if (ret == NULL)
    {
        LOG_ERR("HTTP response for BAD_REQUEST is NULL");
        goto exit;
    }

    const char *query = req->query;
    const char *prefix = "msg=";
    const char *msg_start = strstr(query, prefix);

    if (msg_start)
    {
        msg_start += strlen(prefix);

        /* Find the end of the value (next '&' or end of string) */
        const char *msg_end = msg_start;
        while (*msg_end && *msg_end != '&')
        {
            msg_end++;
        }

        size_t msg_len = msg_end - msg_start;
        if (msg_len > 0)
        {
            char msg_buf[SSS_MAX_SECRET_LEN + 1];
            size_t copy_len = msg_len;
            if (copy_len >= sizeof(msg_buf))
            {
                copy_len = sizeof(msg_buf) - 1;
            }

            char raw[sizeof(msg_buf)];
            memcpy(raw, msg_start, copy_len);
            raw[copy_len] = '\0';

            url_decode(msg_buf, raw, sizeof(msg_buf));

            LOG_INF("Secret to encrypt: %s", msg_buf);

            struct sss_share shares[SSS_N];
            size_t shares_count = 0;
            if (handler_shamir_split(msg_buf, shares, &shares_count) < 0)
            {
                LOG_ERR("Failed to encrypt secret");
                goto exit;
            }

            ret = handler_shares_json_response(shares);
        }
        else
        {
            LOG_ERR("Empty 'msg' parameter");
        }
    }
    else
    {
        LOG_ERR("Missing 'msg' parameter");
    }

exit:
    return (ret);
}

const char *handler_reconstruct(const struct http_request *req)
{
    const char *ret = http_responses_list[HTTP_RESPONSE_BAD_REQUEST];
    const char *query = req->query;
    static char d_buf[2048];
    static char x_buf[64];
    d_buf[0] = '\0';
    x_buf[0] = '\0';

    if (get_query_param(query, "d", d_buf, sizeof(d_buf)) != 0 ||
        get_query_param(query, "x", x_buf, sizeof(x_buf)) != 0)
    {
        goto exit;
    }

    /* URL-decode in case the client sent %2C etc. */
    static char d_dec[2048];
    static char x_dec[64];
    url_decode(d_dec, d_buf, sizeof(d_dec));
    url_decode(x_dec, x_buf, sizeof(x_dec));

    // Count and check the number of shares provided and validate
    int count = count_tokens(d_dec);
    if (count < 2 || count != count_tokens(x_dec))
    {
        goto exit;
    }

    if (count > SSS_N)
    {
        goto exit;
    }

    static struct sss_share shares[SSS_N];

    // Parse entry shares from query parameters (get the shares and their x-coordinates)
    if (handler_parse_entry_share(d_dec, x_dec, shares, count) != 0)
    {
        goto exit;
    }

    /* Reconstruct the secret */
    static uint8_t secret[SSS_MAX_SECRET_LEN];

    if (sss_combine(shares, (unsigned int)count, secret, shares[0].len) != 0)
    {
        ret = http_responses_list[HTTP_RESPONSE_INTERNAL_SERVER_ERROR];
        goto exit;
    }

    /* Build JSON response */
    ret = handler_combine_json_response((const char *)secret);

exit:
    return (ret);
}

// ===========================================================================
// Static functions
// ===========================================================================
static void url_decode(char *dst, const char *src, size_t dst_size)
{
    size_t i = 0;
    while (*src && i < dst_size - 1)
    {
        if (*src == '%' && *(src + 1) && *(src + 2))
        {
            char hex[3] = {src[1], src[2], '\0'};
            dst[i++] = (char)strtol(hex, NULL, 16);
            src += 3;
        }
        else if (*src == '+')
        {
            dst[i++] = ' ';
            src++;
        }
        else
        {
            dst[i++] = *src++;
        }
    }
    dst[i] = '\0';
}

static int handler_shamir_split(const char *secret, struct sss_share *shares_out, size_t *shares_count)
{
    int ret = -1; // error by default
    struct sss_share shares[SSS_N]; // local buffer for shares

    /* --- Shamir's Secret Sharing --- */
    size_t secret_len = strlen(secret);
    if (secret_len > 0 && secret_len <= SSS_MAX_SECRET_LEN)
    {
        ret = sss_split((const uint8_t *)secret, secret_len, SSS_N, SSS_K, shares);
        if (ret != 0)
        {
            goto exit; /* error during split */
        }

        memcpy(shares_out, shares, sizeof(shares));
        *shares_count = SSS_N;
        LOG_DBG("SSS split OK — %u shares (threshold %u)", SSS_N, SSS_K);
    }
    else
    {
        LOG_WRN("Secret too long (%zu bytes, max %u)", secret_len, SSS_MAX_SECRET_LEN);
    }

exit:
    return (ret);
}

static int handler_parse_entry_share(const char *d_dec,
                                     const char *x_dec,
                                     struct sss_share *out_shares,
                                     uint8_t required_shares)
{
    int ret = -1; // error by default

    for (int i = 0; i < required_shares; i++)
    {
        static char hex_token[SSS_MAX_SECRET_LEN * 2 + 1];
        static char x_token[8];
        hex_token[0] = '\0';
        x_token[0] = '\0';

        // Parse the i-th share from the comma-separated lists
        if (get_token(d_dec, i, hex_token, sizeof(hex_token)) != 0 ||
            get_token(x_dec, i, x_token, sizeof(x_token)) != 0)
        {
            goto exit;
        }

        /* Parse x value */
        char *endptr;
        long x_val = strtol(x_token, &endptr, 10);
        if (*endptr != '\0' || x_val < 1 || x_val > 255)
        {
            goto exit;
        }

        out_shares[i].x = (uint8_t)x_val;

        /* Parse hex data length (share length = hex chars / 2) */
        size_t hex_len = strlen(hex_token);
        if (hex_len % 2 != 0 || hex_len == 0 || hex_len / 2 > SSS_MAX_SECRET_LEN)
        {
            goto exit;
        }
        out_shares[i].len = hex_len / 2;

        /* Convert hex to bytes */
        if (hex_to_bytes(hex_token, out_shares[i].data, SSS_MAX_SECRET_LEN) < 0)
        {
            goto exit;
        }
    }

    ret = 0; // success
exit:
    return ret;
}

static const char *handler_shares_json_response(const struct sss_share *shares)
{
    /* Stack buffer for the JSON body (large enough for SSS_N shares of max hex length) */
    char body[SSS_N * (SSS_MAX_SECRET_LEN * 2 + 20) + 2];
    char *p = body;
    size_t room = sizeof(body);
    int n;

    /* JSON array opening */
    n = snprintf(p, room, "[");
    if (n > 0)
    {
        p += n;
        room -= n;
    }

    for (unsigned int i = 0; i < SSS_N; i++)
    {
        /* Convert share data to hex string */
        char hex[SSS_MAX_SECRET_LEN * 2 + 1] = {0};
        for (size_t j = 0; j < shares[i].len; j++)
        {
            size_t pos = strlen(hex);
            snprintf(hex + pos, sizeof(hex) - pos, "%02x", shares[i].data[j]);
        }

        n = snprintf(p, room, "%s{\"x\":%u,\"d\":\"%s\"}", (i > 0) ? "," : "", shares[i].x, hex);
        if (n > 0)
        {
            p += n;
            room -= n;
        }
    }

    /* JSON array closing */
    n = snprintf(p, room, "]");
    if (n > 0)
    {
        p += n;
        room -= n;
    }

    size_t body_len = p - body;

    /* Build full HTTP response with the JSON body */
    static char http_resp[4096];
    snprintf(http_resp, sizeof(http_resp), http_responses_list[HTTP_RESPONSE_JSON_OK], body_len, body);

    LOG_DBG("JSON response: %s", body);

    return (http_resp);
}

static const char *handler_combine_json_response(const char *secret)
{
    static char http_resp[512];
    char body[SSS_MAX_SECRET_LEN + 20]; /* stack buffer for JSON body */

    int n = snprintf(body, sizeof(body), "{\"secret\":\"%s\"}", secret);
    size_t body_len = (n > 0) ? (size_t)n : 0;

    snprintf(http_resp, sizeof(http_resp), http_responses_list[HTTP_RESPONSE_JSON_OK], body_len, body);

    LOG_DBG("JSON response: %s", body);

    return (http_resp);
}

static int hex_to_bytes(const char *hex, uint8_t *out, size_t out_max)
{
    size_t len = strlen(hex);
    if (len % 2 != 0 || len / 2 > out_max)
    {
        return -1;
    }

    for (size_t i = 0; i < len; i += 2)
    {
        char byte_str[3] = {hex[i], hex[i + 1], '\0'};
        char *endptr;
        long b = strtol(byte_str, &endptr, 16);
        if (*endptr != '\0')
        {
            return -1;
        }
        out[i / 2] = (uint8_t)b;
    }
    return (int)(len / 2);
}

static int get_query_param(const char *query, const char *param, char *out, size_t out_size)
{
    size_t param_len = strlen(param);
    const char *start = query;

    while (1)
    {
        start = strstr(start, param);
        if (start == NULL)
            return -1;

        /* Check that it's the beginning of a param (at query start or after '&') */
        if (start == query || *(start - 1) == '&')
        {
            if (start[param_len] == '=')
            {
                start += param_len + 1;
                break;
            }
        }
        start += param_len;
    }

    const char *end = start;
    while (*end && *end != '&')
    {
        end++;
    }

    size_t val_len = end - start;
    if (val_len >= out_size)
    {
        val_len = out_size - 1;
    }

    memcpy(out, start, val_len);
    out[val_len] = '\0';

    return 0;
}

static int count_tokens(const char *s)
{
    if (s == NULL || *s == '\0')
        return 0;
    int n = 1;
    while (*s)
    {
        if (*s == ',')
            n++;
        s++;
    }
    return n;
}

static int get_token(const char *s, int index, char *out, size_t out_size)
{
    int current = 0;
    const char *start = s;

    while (1)
    {
        if (current == index)
        {
            const char *end = start;
            while (*end && *end != ',')
            {
                end++;
            }
            size_t len = end - start;
            if (len >= out_size)
            {
                len = out_size - 1;
            }
            memcpy(out, start, len);
            out[len] = '\0';
            return 0;
        }

        while (*start && *start != ',')
        {
            start++;
        }
        if (*start == '\0')
        {
            return -1; /* not enough tokens */
        }
        start++; /* skip ',' */
        current++;
    }
}
