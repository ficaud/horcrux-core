/**
 * @file wifi_mgr.c
 *
 * @brief Wi-Fi Access Point management and event handling.
 *
 * @author Julien F.
 * @date 2026-07-12
 *
 * @details This module handles the initialization and lifecycle of the Wi-Fi
 *          Access Point (AP) on the ESP32-S3. It registers a network management
 *          event callback to monitor Wi-Fi state transitions (station connect/
 *          disconnect, AP enable/disable) and coordinates AP startup using a
 *          semaphore-based synchronization pattern. The AP configuration settings
 *          are defined in prj.conf.
 */

#include "wifi_mgr.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/wifi_mgmt.h>

#include "dhcp.h"

// ===========================================================================
// Zephyr logging module registration
// ===========================================================================
LOG_MODULE_REGISTER(wifi_mgr, LOG_LEVEL_INF);
// ===========================================================================
// Structure and variables definition
// ===========================================================================
//  Assertions of configuration (in prj.conf)
BUILD_ASSERT(sizeof(CONFIG_WIFI_SAMPLE_AP_SSID) > 1,
             "CONFIG_WIFI_SAMPLE_AP_SSID is empty. Please set it in conf file.");

// Semaphore to signal when AP mode is ready
static struct k_sem ap_ready_sem;
// ===========================================================================
// Static function declarations
// ===========================================================================
/**
 * @brief Callback function to handle Wi-Fi events.
 *
 * @param cb[in] : Pointer to the net_mgmt_event_callback structure.
 * @param mgmt_event[in] : The management event that occurred.
 * @param iface[in] : Pointer to the network interface associated with the event.
 *
 * @return void
 */
static void wifi_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event, struct net_if *iface);

/**
 * @brief Enable Wi-Fi Access Point (AP) mode.
 *
 * @param ap_iface[in] : Pointer to the network interface for the AP.
 *
 * @return 0 on success, negative error code on failure.
 */
static int enable_ap_mode(struct net_if **ap_iface);

// ===========================================================================
// Public function definition
// ===========================================================================

int wifi_mgr_init(void)
{
    int ret = -1; // error by default
    static struct net_if *ap_iface;
    static struct net_mgmt_event_callback cb;
    // Delay to allow the Wi-Fi driver to initialize
    k_sleep(K_SECONDS(5));

    // Semaphor initialization for AP ready signal
    ret = k_sem_init(&ap_ready_sem, 0, 1);
    if (ret)
    {
        LOG_ERR("Failed to initialize semaphore, err: %d", ret);
        goto exit;
    }

    // Register Wi-Fi event callback
    net_mgmt_init_event_callback(&cb, wifi_event_handler, NET_EVENT_WIFI_MASK);
    net_mgmt_add_event_callback(&cb);

    // Get wifi AP interface
    ap_iface = net_if_get_wifi_sap();

    if (!ap_iface)
    {
        LOG_ERR("Failed to get Wi-Fi AP interface");
        goto exit;
    }

    // Enable AP mode
    ret = enable_ap_mode(&ap_iface);

    if (ret)
    {
        LOG_ERR("enable_ap_mode failed (%d)", ret);
        goto exit;
    }

    // Wait for AP ready signal to be given by the event handler
    ret = k_sem_take(&ap_ready_sem, K_FOREVER);

exit:
    return (ret);
}
// ===========================================================================
// Static function definition
// ===========================================================================
static void wifi_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event, struct net_if *iface)
{
    switch (mgmt_event)
    {
        case NET_EVENT_WIFI_CONNECT_RESULT:
            LOG_INF("Connected to %s", CONFIG_WIFI_SAMPLE_SSID);
            break;

        case NET_EVENT_WIFI_DISCONNECT_RESULT:
            LOG_INF("Disconnected from %s", CONFIG_WIFI_SAMPLE_SSID);
            break;

        case NET_EVENT_WIFI_AP_ENABLE_RESULT:
            struct wifi_status *status = (struct wifi_status *)cb->info;
            if (status->status)
            {
                LOG_ERR("AP enable failed (%d)", status->status);
            }
            else
            {
                LOG_INF("AP Mode enabled");
                k_sem_give(&ap_ready_sem);
            }
            break;

        case NET_EVENT_WIFI_AP_DISABLE_RESULT:
            LOG_INF("AP Mode disabled");
            break;

        case NET_EVENT_WIFI_AP_STA_CONNECTED:
        {
            struct wifi_ap_sta_info *sta = (struct wifi_ap_sta_info *)cb->info;
            LOG_INF("station: " MACSTR " joined",
                    sta->mac[0],
                    sta->mac[1],
                    sta->mac[2],
                    sta->mac[3],
                    sta->mac[4],
                    sta->mac[5]);
            break;
        }

        case NET_EVENT_WIFI_AP_STA_DISCONNECTED:
        {
            struct wifi_ap_sta_info *sta = (struct wifi_ap_sta_info *)cb->info;
            LOG_INF("station: " MACSTR " leave",
                    sta->mac[0],
                    sta->mac[1],
                    sta->mac[2],
                    sta->mac[3],
                    sta->mac[4],
                    sta->mac[5]);
            break;
        }

        default:
            break;
    }
}

static int enable_ap_mode(struct net_if **ap_iface)
{
    struct wifi_connect_req_params ap_config;

    if (!ap_iface || !*ap_iface)
    {
        LOG_ERR("AP interface not initialized");
        return -EIO;
    }

    LOG_INF("Turning on AP Mode");

    ap_config.ssid = (const uint8_t *)CONFIG_WIFI_SAMPLE_AP_SSID;
    ap_config.ssid_length = sizeof(CONFIG_WIFI_SAMPLE_AP_SSID) - 1;
    ap_config.psk = (const uint8_t *)CONFIG_WIFI_SAMPLE_AP_PSK;
    ap_config.psk_length = sizeof(CONFIG_WIFI_SAMPLE_AP_PSK) - 1;
    ap_config.channel = WIFI_CHANNEL_ANY;
    ap_config.band = WIFI_FREQ_BAND_2_4_GHZ;
    ap_config.security = (sizeof(CONFIG_WIFI_SAMPLE_AP_PSK) == 1) ? WIFI_SECURITY_TYPE_NONE : WIFI_SECURITY_TYPE_PSK;

    int ret = net_mgmt(NET_REQUEST_WIFI_AP_ENABLE, *ap_iface, &ap_config, sizeof(struct wifi_connect_req_params));
    if (ret)
    {
        LOG_ERR("NET_REQUEST_WIFI_AP_ENABLE failed, err: %d", ret);
        return ret;
    }

#if CONFIG_WIFI_SAMPLE_DHCPV4_START
    /* Start DHCP server only after AP is fully enabled */
    dhcp_server_start(*ap_iface);
#endif

    return 0;
}
