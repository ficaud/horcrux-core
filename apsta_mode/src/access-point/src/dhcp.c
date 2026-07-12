/*
 * DHCP — IP configuration and DHCPv4 server startup on the AP interface.
 */

#include "dhcp.h"

#include <zephyr/logging/log.h>
#include <zephyr/net/dhcpv4_server.h>
#include <zephyr/net/net_ip.h>

LOG_MODULE_REGISTER(dhcp, LOG_LEVEL_INF);

/* ---------------------------------------------------------------------------
 * Configuration assertions
 * ------------------------------------------------------------------------- */

BUILD_ASSERT(sizeof(CONFIG_WIFI_SAMPLE_AP_IP_ADDRESS) > 1,
             "CONFIG_WIFI_SAMPLE_AP_IP_ADDRESS is empty. Please set it in conf file.");
BUILD_ASSERT(sizeof(CONFIG_WIFI_SAMPLE_AP_NETMASK) > 1,
             "CONFIG_WIFI_SAMPLE_AP_NETMASK is empty. Please set it in conf file.");

/* ---------------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------------- */

int dhcp_server_start(struct net_if *iface)
{
    struct in_addr addr;
    struct in_addr netmaskAddr;

    if (net_addr_pton(AF_INET, CONFIG_WIFI_SAMPLE_AP_IP_ADDRESS, &addr))
    {
        LOG_ERR("Invalid address: %s", CONFIG_WIFI_SAMPLE_AP_IP_ADDRESS);
        return -EINVAL;
    }
    if (net_addr_pton(AF_INET, CONFIG_WIFI_SAMPLE_AP_NETMASK, &netmaskAddr))
    {
        LOG_ERR("Invalid netmask: %s", CONFIG_WIFI_SAMPLE_AP_NETMASK);
        return -EINVAL;
    }

    net_if_ipv4_set_gw(iface, &addr);

    if (net_if_ipv4_addr_add(iface, &addr, NET_ADDR_MANUAL, 0) == NULL)
    {
        LOG_ERR("Unable to set IP address for AP interface");
        return -EIO;
    }
    if (!net_if_ipv4_set_netmask_by_addr(iface, &addr, &netmaskAddr))
    {
        LOG_ERR("Unable to set netmask: %s", CONFIG_WIFI_SAMPLE_AP_NETMASK);
        return -EIO;
    }

    addr.s4_addr[3] += 10; /* Début du pool DHCP */

    if (net_dhcpv4_server_start(iface, &addr) != 0)
    {
        LOG_ERR("DHCP server failed to start");
        return -EIO;
    }

    LOG_INF("DHCPv4 server started on %s", CONFIG_WIFI_SAMPLE_AP_IP_ADDRESS);
    return 0;
}
