#ifndef ACCESS_POINT_DHCP_H
#define ACCESS_POINT_DHCP_H

#include <zephyr/net/net_if.h>

/**
 * @brief Configure the IP address and start the DHCPv4 server on the
 *        AP interface.
 *
 * @param iface Wi-Fi AP interface (obtained via net_if_get_wifi_sap()).
 * @return 0 on success, negative error code on failure.
 */
int dhcp_server_start(struct net_if *iface);

#endif /* ACCESS_POINT_DHCP_H */
