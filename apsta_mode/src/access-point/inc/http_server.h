#ifndef ACCESS_POINT_HTTP_SERVER_H
#define ACCESS_POINT_HTTP_SERVER_H

/**
 * @brief Start the captive portal HTTP server (dedicated thread, TCP port 80).
 *
 * @return 0 on success, negative error code on failure.
 */
int http_server_start(void);

/**
 * @brief Stop the HTTP server and release resources.
 *
 * @return 0 on success, negative error code on failure.
 */
int http_server_stop(void);

#endif /* ACCESS_POINT_HTTP_SERVER_H */
