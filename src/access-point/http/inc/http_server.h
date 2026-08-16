#ifndef ACCESS_POINT_HTTP_SERVER_H
#define ACCESS_POINT_HTTP_SERVER_H

// ===========================================================================
// Definitions
// ===========================================================================
#define HTTP_PORT        (80)
#define HTTP_BACKLOG     (5)
/* Sized for on-device QR decoding. With QUIRC_MAX_PAYLOAD=2048 in the
 * firmware build, quirc_decode() keeps ~2 KB on the stack (struct datastream)
 * on top of the ~4 KB HTTP receive frame. The classic ESP32's dram1 region
 * is only 96 KB, so the stack must stay small. */
#define HTTP_STACK_SIZE  (10240)
#define TLS_STACK_SIZE   (2048)
#define HTTP_PRIORITY    (5)
#define HTTP_RX_BUF_SIZE (2048)

// ===========================================================================
// Public function declaration
// ===========================================================================
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
