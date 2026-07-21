#include <zephyr/logging/log.h>

#include "dns.h"
#include "http_server.h"
#include "wifi_mgr.h"

LOG_MODULE_REGISTER(MAIN);

/* ---------------------------------------------------------------------------
 * main
 * ------------------------------------------------------------------------- */

int main(void)
{
    int ret;
    // Wifi manager initlialization for AP mode
    ret = wifi_mgr_init();
    LOG_INF("Wi-Fi AP mode initialized, ret: %d", ret);

    // DNS interceptor start
    dns_interceptor_start();

    // HTTP server start
    http_server_start();

    LOG_INF("Captive portal started");
    return 0;
}
