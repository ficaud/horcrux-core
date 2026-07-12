#ifndef ACCESS_POINT_HANDLERS_H
#define ACCESS_POINT_HANDLERS_H

#include "router.h"

/**
 * @brief Root handler — captive portal home page.
 */
const char *handler_root(const struct http_request *req);

/**
 * @brief Captive portal handler — used for all probes
 *        (Android, Apple, Windows) and any unknown route.
 *        Returns the HTML page that triggers the popup.
 */
const char *handler_captive_portal(const struct http_request *req);

#endif /* ACCESS_POINT_HANDLERS_H */
