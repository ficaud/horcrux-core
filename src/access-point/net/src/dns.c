/**
 * @file dns.c
 *
 * @brief DNS interceptor for the captive portal.
 *
 * @author Julien F.
 * @date 2026-07-12
 *
 * @details This module implements a simple DNS interceptor for the captive portal.
 *          It listens for DNS queries on port 53 and responds with the portal IP
 *          address, effectively redirecting all DNS requests to the captive portal.
 */

#include "dns.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>

#include <errno.h>
#include <string.h>

// ===========================================================================
// Zephyr logging module registration
// ===========================================================================
LOG_MODULE_REGISTER(dns, LOG_LEVEL_INF);

// ===========================================================================
// Structure and variables definition
// ===========================================================================
K_THREAD_STACK_DEFINE(dns_stack, DNS_STACK_SIZE);
static struct k_thread dns_thread;

// ===========================================================================
// Static function declarations
// ===========================================================================
/**
 * @brief Build a DNS response packet for the given query.
 *
 * @param buf[in]       Buffer to write the response into.
 * @param query[in]     The original DNS query packet.
 * @param query_len[in] Length of the original query packet.
 *
 * @return void
 */
static void build_dns_response(uint8_t *buf, const uint8_t *query, int query_len);

/**
 * @brief DNS interceptor thread function.
 *
 * Listens for DNS queries on UDP port 53 and responds with the captive portal IP.
 *
 * @param arg1 Unused.
 * @param arg2 Unused.
 * @param arg3 Unused.
 *
 * @return void
 */
static void dns_thread_fn(void *arg1, void *arg2, void *arg3);
// ===========================================================================
// Public function definition
// ===========================================================================
int dns_interceptor_start(void)
{
    k_thread_create(
        &dns_thread, dns_stack, DNS_STACK_SIZE, dns_thread_fn, NULL, NULL, NULL, DNS_PRIORITY, 0, K_NO_WAIT);
    return 0;
}

// ===========================================================================
// Static function definition
// ===========================================================================
static void build_dns_response(uint8_t *buf, const uint8_t *query, int query_len)
{
    /*
     * DNS packet format (RFC 1035 §4.1):
     *
     *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
     *  |                      HEADER                     |  12 bytes
     *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
     *  |                      QUESTION                   |  variable
     *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
     *  |                       ANSWER                    |  variable
     *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
     *
     * We copy the query as-is, then overwrite the header fields
     * and append a single A-record answer pointing to the portal IP.
     */
    const uint8_t portal_ip[4] = {192, 168, 4, 1};
    memcpy(buf, query, query_len);

    /* --- Header (RFC 1035 §4.1.1) --- */

    /* Flags: QR=1 (response), OPCODE=0000 (standard query),
     * AA=0, TC=0, RD=1 (copied from query), RA=1, Z=000, RCODE=0000 (no error) */
    buf[2] = 0x81;
    buf[3] = 0x80;

    /* ANCOUNT = 1 answer section record */
    buf[6] = 0x00;
    buf[7] = 0x01;

    /* --- Answer section (RFC 1035 §4.1.3) --- */

    /* Name pointer (RFC 1035 §4.1.4): 0xC0 | 0x0C means
     * "domain name same as at offset 12 (the query name)" */
    int answer_pos = query_len;

    buf[answer_pos++] = 0xC0;
    buf[answer_pos++] = 0x0C;

    /* TYPE = A (0x0001) — IPv4 host address */
    buf[answer_pos++] = 0x00;
    buf[answer_pos++] = 0x01;

    /* CLASS = IN (0x0001) — Internet */
    buf[answer_pos++] = 0x00;
    buf[answer_pos++] = 0x01;

    /* TTL = 300 seconds (5 minutes) */
    buf[answer_pos++] = 0x00;
    buf[answer_pos++] = 0x00;
    buf[answer_pos++] = 0x01;
    buf[answer_pos++] = 0x2C;

    /* RDLENGTH = 4 bytes (IPv4 address) */
    buf[answer_pos++] = 0x00;
    buf[answer_pos++] = 0x04;

    /* RDATA = portal IP (192.168.4.1) */
    memcpy(&buf[answer_pos], portal_ip, 4);
}

static void dns_thread_fn(void *arg1, void *arg2, void *arg3)
{
    int sock;
    struct sockaddr_in addr;
    uint8_t rx_buf[DNS_BUF_SIZE];
    uint8_t tx_buf[DNS_BUF_SIZE];
    int rx_len;

    // Create UDP socket for DNS
    sock = zsock_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0)
    {
        LOG_ERR("socket failed (%d)", errno);
        return;
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(DNS_PORT);

    // Bind the socket to UDP port 53
    if (zsock_bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        LOG_ERR("bind failed (%d)", errno);
        zsock_close(sock);
        return;
    }

    LOG_INF("DNS interceptor ready on port %d", DNS_PORT);

    while (1)
    {
        struct sockaddr_in client;
        socklen_t client_len = sizeof(client);

        // Receive DNS query
        rx_len = zsock_recvfrom(sock, rx_buf, sizeof(rx_buf), 0, (struct sockaddr *)&client, &client_len);
        if (rx_len < 12)
        {
            continue;
        }

        // only handle standard A queries (QR=0, OPCODE=0)
        if (rx_buf[2] != 0x01 || rx_buf[3] != 0x00)
        {
            continue;
        }

        // Build and send DNS response
        build_dns_response(tx_buf, rx_buf, rx_len);

        // Send the response back to the client
        int tx_len = rx_len + 16;
        zsock_sendto(sock, tx_buf, tx_len, 0, (struct sockaddr *)&client, client_len);
    }

    // Cleanup (unreachable in this infinite loop, but good practice)
    zsock_close(sock);
}
