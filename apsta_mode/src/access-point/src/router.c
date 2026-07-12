/**
 * @file router.c
 *
 * @brief HTTP request routing for the captive portal.
 *
 * @author Julien F.
 * @date 2026-07-12
 *
 * @details This module handles the routing of HTTP requests
 *          for the captive portal.
 */

#include "router.h"

#include "handlers.h"

#include <string.h>

// ===========================================================================
// Variables definition
// ===========================================================================
static const struct
{
    const char *path;
    handler_fn handler;
} routes[] = {
    {"/", handler_root},
    /* All other paths fall through to handler_captive_portal */
};

// ===========================================================================
// Public function definition
// ===========================================================================
int router_parse(struct http_request *req, char *raw, size_t raw_len)
{
    int ret = -1;
    char *ptr = raw;
    char *end = raw + raw_len;

    while (ptr < end && *ptr != ' ')
    {
        ptr++;
    }

    if (ptr >= end)
    {
        goto exit;
    }

    size_t method_len = ptr - raw;

    if (method_len >= ROUTER_METHOD_MAX)
    {
        method_len = ROUTER_METHOD_MAX - 1;
    }

    memcpy(req->method, raw, method_len);

    req->method[method_len] = '\0';

    ptr++;
    char *path_start = ptr;

    while (ptr < end && *ptr != ' ' && *ptr != '?')
    {
        ptr++;
    }

    size_t path_len = ptr - path_start;

    if (path_len >= ROUTER_PATH_MAX)
    {
        path_len = ROUTER_PATH_MAX - 1;
    }

    memcpy(req->path, path_start, path_len);
    req->path[path_len] = '\0';

    /* ── Body (after \r\n\r\n) ── */

    char *body_start = strstr(raw, "\r\n\r\n");
    if (body_start)
    {
        body_start += 4;
        req->body = body_start;
        req->body_len = raw_len - (body_start - raw);
    }
    else
    {
        req->body = NULL;
        req->body_len = 0;
    }

    ret = 0;
exit:
    return ret;
}

const char *router_dispatch(const struct http_request *req)
{
    size_t route_count = sizeof(routes) / sizeof(routes[0]);

    for (size_t i = 0; i < route_count; i++)
    {
        if (strcmp(req->path, routes[i].path) == 0)
        {
            return routes[i].handler(req);
        }
    }

    return handler_captive_portal(req);
}
