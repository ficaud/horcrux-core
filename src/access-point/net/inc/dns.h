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
 * The DNS interceptor will answer A-record queries for this domain,
 * returning the captive portal IP.
 */
#define DNS_PORTAL_DOMAIN "relic.co"

/**
 * @brief Well-known captive portal detection domains.
 *
 * When a device connects to a Wi-Fi network, the OS (Android, iOS, macOS,
 * Windows) checks for a captive portal by resolving one of these domains and
 * requesting a well-known path (e.g. /generate_204, /hotspot-detect.html).
 * Answering these with the portal IP makes the OS trigger the captive portal
 * popup. All other queries are left unanswered (dropped) so that normal DNS
 * resolution is not intercepted.
 */
#define DNS_PORTAL_DETECTION_DOMAINS                                                             \
    "connectivitycheck.gstatic.com",                                                             \
    "connectivitycheck.android.com",                                                             \
    "clients3.google.com",                                                                       \
    "captive.apple.com",                                                                         \
    "gsp1.apple.com",                                                                            \
    "www.msftconnecttest.com",                                                                   \
    "ipv6.msftconnecttest.com",                                                                  \
    "neverssl.com",                                                                              \
    "network-test.com"

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
