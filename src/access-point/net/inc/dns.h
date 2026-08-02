#ifndef ACCESS_POINT_DNS_H
#define ACCESS_POINT_DNS_H

// ===========================================================================
// Definitions
// ===========================================================================
#define DNS_PORT         (53)
#define DNS_STACK_SIZE   (4096)
#define DNS_PRIORITY     (5)
#define DNS_BUF_SIZE     (256)
#define DNS_NAME_MAX_LEN (64) // maximum length of a DNS name
#define DNS_HEADER_LEN   (12)
/**
 * @brief Domain name the DNS interceptor responds to (captive portal domain).
 *
 * The DNS interceptor will only answer A-record queries for this domain,
 * returning the captive portal IP. All other queries are left unanswered
 * (dropped), so that normal DNS resolution is not intercepted.
 */
#define DNS_PORTAL_DOMAIN "horcrux.co"

// ===========================================================================
// Public function declaration
// ===========================================================================
/**
 * @brief Start the DNS interceptor (dedicated thread, UDP port 53).
 *        Only DNS queries for DNS_PORTAL_DOMAIN receive the captive portal IP.
 *        All other queries are left unanswered.
 *
 * @return 0 on success, negative error code on failure.
 */
int dns_interceptor_start(void);

#endif /* ACCESS_POINT_DNS_H */
