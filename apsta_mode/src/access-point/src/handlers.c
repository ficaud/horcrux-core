/**
 * @file handlers.c
 *
 * @brief HTTP request handlers for the captive portal.
 *
 * @author Julien F.
 * @date 2026-07-12
 *
 * @details This module implements HTTP request handlers for the captive portal.
 *          It serves the appropriate HTML pages based on the request path.
 */

#include "handlers.h"

#include "page_captive.h"

// ===========================================================================
// Variables definition
// ===========================================================================
const char *handler_root(const struct http_request *req)
{
    (void)req;
    return page_captive;
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
