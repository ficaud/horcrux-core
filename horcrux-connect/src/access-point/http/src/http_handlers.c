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
const char *handler_encrypt(const struct http_request *req)
{
    const char *resp = http_responses_list[HTTP_RESPONSE_COUNT_INVALID]; // http resp error by default
    /* Parse the 'msg' query parameter */
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
            /* Copy to temporary buffer and URL-decode */
            char msg_buf[128];
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
        }
    }

    resp = http_responses_list[HTTP_RESPONSE_OK]; // return 200 OK

    return (resp);
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
