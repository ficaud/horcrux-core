#ifndef ACCESS_POINT_DNS_H
#define ACCESS_POINT_DNS_H

// ===========================================================================
// Definitions
// ===========================================================================
#define DNS_PORT       (53)
#define DNS_STACK_SIZE (4096)
#define DNS_PRIORITY   (5)
#define DNS_BUF_SIZE   (256)

// ===========================================================================
// Public function declaration
// ===========================================================================
/**
 * @brief Start the DNS interceptor (dedicated thread, UDP port 53).
 *        Any DNS A-record query receives the captive portal IP.
 *
 * @return 0 on success, negative error code on failure.
 */
int dns_interceptor_start(void);

#endif /* ACCESS_POINT_DNS_H */
