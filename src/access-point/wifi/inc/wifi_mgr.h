#ifndef ACCESS_POINT_WIFI_MGR_H
#define ACCESS_POINT_WIFI_MGR_H

// ===========================================================================
// Definitions
// ===========================================================================
#define MACSTR "%02X:%02X:%02X:%02X:%02X:%02X"

// Event mask for Wi-Fi events we are interested in
#define NET_EVENT_WIFI_MASK                                                                                            \
    (NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT | NET_EVENT_WIFI_AP_ENABLE_RESULT |              \
     NET_EVENT_WIFI_AP_DISABLE_RESULT | NET_EVENT_WIFI_AP_STA_CONNECTED | NET_EVENT_WIFI_AP_STA_DISCONNECTED)

// ===========================================================================
// Public function declaration
// ===========================================================================
/**
 * @brief Initialise Wi-Fi in Access Point (AP) mode.
 *
 * Steps:
 *  1. Wait 5s for the Wi-Fi driver to initialise
 *  2. Register the Wi-Fi event callback
 *  3. Get the AP interface (net_if_get_wifi_sap)
 *  4. Configure SSID/PSK/channel and enable AP
 *  5. Start the DHCPv4 server
 *  6. Block until NET_EVENT_WIFI_AP_ENABLE_RESULT is received
 *
 * @return 0 on success (AP ready), negative error code on failure.
 */
int wifi_mgr_init(void);

#endif /* ACCESS_POINT_WIFI_MGR_H */
