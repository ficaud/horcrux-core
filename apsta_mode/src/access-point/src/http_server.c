/*
 * HTTP Server — serves the captive portal pages.
 * Delegates routing to the router module and content to the handlers module.
 */

#include "http_server.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>

#include "page_captive.h"
#include "router.h"

#include <errno.h>
#include <string.h>

LOG_MODULE_REGISTER(http, LOG_LEVEL_INF);

/* ---------------------------------------------------------------------------
 * Configuration
 * ------------------------------------------------------------------------- */

#define HTTP_PORT        80
#define HTTP_BACKLOG     5
#define HTTP_STACK_SIZE  4096
#define HTTP_PRIORITY    5
#define HTTP_RX_BUF_SIZE 512

/* ---------------------------------------------------------------------------
 * HTTP thread
 * ------------------------------------------------------------------------- */

static void http_thread_fn(void *arg1, void *arg2, void *arg3)
{
    int server_fd, client_fd;
    struct sockaddr_in addr;
    int opt = 1;

    server_fd = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_fd < 0)
    {
        LOG_ERR("socket failed (%d)", errno);
        return;
    }

    zsock_setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(HTTP_PORT);

    if (zsock_bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        LOG_ERR("bind failed (%d)", errno);
        zsock_close(server_fd);
        return;
    }

    if (zsock_listen(server_fd, HTTP_BACKLOG) < 0)
    {
        LOG_ERR("listen failed (%d)", errno);
        zsock_close(server_fd);
        return;
    }

    LOG_INF("HTTP server ready on port %d", HTTP_PORT);

    while (1)
    {
        char rx_buf[HTTP_RX_BUF_SIZE];
        struct http_request req;
        const char *response;

        client_fd = zsock_accept(server_fd, NULL, NULL);
        if (client_fd < 0)
        {
            LOG_ERR("accept failed (%d)", errno);
            continue;
        }

        /* Read the request */
        int n = zsock_recv(client_fd, rx_buf, sizeof(rx_buf) - 1, 0);
        if (n > 0)
        {
            rx_buf[n] = '\0';

            /* Parse + dispatch */
            if (router_parse(&req, rx_buf, n) == 0)
            {
                response = router_dispatch(&req);
            }
            else
            {
                response = "HTTP/1.1 400 Bad Request\r\n"
                           "Content-Type: text/plain\r\n"
                           "Connection: close\r\n"
                           "\r\n"
                           "Bad Request";
            }
        }
        else
        {
            response = "HTTP/1.1 400 Bad Request\r\n"
                       "Content-Type: text/plain\r\n"
                       "Connection: close\r\n"
                       "\r\n"
                       "Empty Request";
        }

        zsock_send(client_fd, response, strlen(response), 0);
        zsock_close(client_fd);
    }
}

/* ---------------------------------------------------------------------------
 * Thread resources
 * ------------------------------------------------------------------------- */

K_THREAD_STACK_DEFINE(http_stack, HTTP_STACK_SIZE);
static struct k_thread http_thread;

/* ---------------------------------------------------------------------------
 * TLS listener thread — port 443 (catches Android HTTPS probes)
 * ------------------------------------------------------------------------- */

static void tls_thread_fn(void *arg1, void *arg2, void *arg3)
{
    int server_fd, client_fd;
    struct sockaddr_in addr;
    int opt = 1;

    server_fd = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_fd < 0)
    {
        LOG_ERR("tls socket failed (%d)", errno);
        return;
    }

    zsock_setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(443);

    if (zsock_bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        LOG_ERR("tls bind failed (%d)", errno);
        zsock_close(server_fd);
        return;
    }

    if (zsock_listen(server_fd, HTTP_BACKLOG) < 0)
    {
        LOG_ERR("tls listen failed (%d)", errno);
        zsock_close(server_fd);
        return;
    }

    LOG_INF("HTTPS server ready on port 443");

    while (1)
    {
        client_fd = zsock_accept(server_fd, NULL, NULL);
        if (client_fd < 0)
        {
            LOG_ERR("tls accept failed (%d)", errno);
            continue;
        }

        /* Just read whatever comes (TLS ClientHello) and discard,
         * then reply with the captive portal page.
         * Android will see a 200 HTML response and trigger the popup. */
        char discard[256];
        zsock_recv(client_fd, discard, sizeof(discard), 0);
        zsock_send(client_fd, PAGE_CAPTIVE, strlen(PAGE_CAPTIVE), 0);
        zsock_close(client_fd);
    }
}

K_THREAD_STACK_DEFINE(https_stack, HTTP_STACK_SIZE);
static struct k_thread https_thread;

/* ---------------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------------- */

int http_server_start(void)
{
    k_thread_create(
        &http_thread, http_stack, HTTP_STACK_SIZE, http_thread_fn, NULL, NULL, NULL, HTTP_PRIORITY, 0, K_NO_WAIT);

    k_thread_create(
        &https_thread, https_stack, HTTP_STACK_SIZE, tls_thread_fn, NULL, NULL, NULL, HTTP_PRIORITY, 0, K_NO_WAIT);

    return 0;
}

int http_server_stop(void)
{
    /* TODO : implémenter un flag d'arrêt + k_thread_abort() */
    return 0;
}
