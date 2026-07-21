/**
 * @file dhcp.c
 *
 * @brief DHCP server for the captive portal.
 *
 * @author Julien F.
 * @date 2026-07-12
 *
 * @details This module implements a simple DHCP server for the captive portal.
 *          It assigns IP addresses to clients connecting to the AP interface.
 */

#include "dhcp.h"

#include <zephyr/logging/log.h>
#include <zephyr/net/dhcpv4_server.h>
#include <zephyr/net/net_ip.h>

// ===========================================================================
// Zephyr logging module registration
// ===========================================================================
LOG_MODULE_REGISTER(dhcp, LOG_LEVEL_INF);

// ===========================================================================
// Structure and variables definition
// ===========================================================================
BUILD_ASSERT(sizeof(CONFIG_WIFI_SAMPLE_AP_IP_ADDRESS) > 1,
             "CONFIG_WIFI_SAMPLE_AP_IP_ADDRESS is empty. Please set it in conf file.");
BUILD_ASSERT(sizeof(CONFIG_WIFI_SAMPLE_AP_NETMASK) > 1,
             "CONFIG_WIFI_SAMPLE_AP_NETMASK is empty. Please set it in conf file.");

// ===========================================================================
// Public function definition
// ===========================================================================
int dhcp_server_start(struct net_if *iface)
{
    int ret = -EINVAL;
    struct in_addr addr;
    struct in_addr netmaskAddr;

    if (net_addr_pton(AF_INET, CONFIG_WIFI_SAMPLE_AP_IP_ADDRESS, &addr))
    {
        LOG_ERR("Invalid address: %s", CONFIG_WIFI_SAMPLE_AP_IP_ADDRESS);
        goto exit;
    }
    if (net_addr_pton(AF_INET, CONFIG_WIFI_SAMPLE_AP_NETMASK, &netmaskAddr))
    {
        LOG_ERR("Invalid netmask: %s", CONFIG_WIFI_SAMPLE_AP_NETMASK);
        goto exit;
    }

    net_if_ipv4_set_gw(iface, &addr);

    if (net_if_ipv4_addr_add(iface, &addr, NET_ADDR_MANUAL, 0) == NULL)
    {
        LOG_ERR("Unable to set IP address for AP interface");
        ret = -EIO;
        goto exit;
    }
    if (!net_if_ipv4_set_netmask_by_addr(iface, &addr, &netmaskAddr))
    {
        LOG_ERR("Unable to set netmask: %s", CONFIG_WIFI_SAMPLE_AP_NETMASK);
        ret = -EIO;
        goto exit;
    }

    addr.s4_addr[3] += 10; /* Début du pool DHCP */

    if (net_dhcpv4_server_start(iface, &addr) != 0)
    {
        LOG_ERR("DHCP server failed to start");
        ret = -EIO;
        goto exit;
    }

    // If we get here, everything is OK
    ret = 0;

    LOG_INF("DHCPv4 server started on %s", CONFIG_WIFI_SAMPLE_AP_IP_ADDRESS);
exit:
    return ret;
}
