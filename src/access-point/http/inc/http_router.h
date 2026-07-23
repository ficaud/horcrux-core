#ifndef ACCESS_POINT_ROUTER_H
#define ACCESS_POINT_ROUTER_H

#include <stddef.h>

// ===========================================================================
// Definitions
// ===========================================================================
#define ROUTER_METHOD_MAX (8)
#define ROUTER_PATH_MAX   (64)
#define ROUTER_QUERY_MAX  (2048)

// ===========================================================================
// Typedef and structure definition
// ===========================================================================
/**
 * @brief Parsed HTTP request.
 *
 * The body / query pointers reference the raw buffer
 * (no dynamic allocation).
 */
struct http_request;

/** Handler function signature */
typedef const char *(*handler_fn)(const struct http_request *);

struct http_request
{
    char method[ROUTER_METHOD_MAX];
    char path[ROUTER_PATH_MAX];
    char query[ROUTER_QUERY_MAX];
    const char *body; /* pointe dans raw (NULL si pas de body) */
    size_t body_len;
};

// ===========================================================================
// Public function declaration
// ===========================================================================
/**
 * @brief Parse a raw HTTP request.
 *
 * Extracts the method, path, and locates the body start.
 * The @p raw buffer is modified in place (null-terminators inserted).
 *
 * @param req[out]     Structure to fill with parsed data.
 * @param raw[in,out]  Buffer containing the raw HTTP request (modified).
 * @param raw_len[in]  Number of bytes received.
 * @return 0 on success, -1 if parsing failed.
 */
int router_parse(struct http_request *req, char *raw, size_t raw_len);

/**
 * @brief Dispatch the request to the appropriate handler.
 *
 * @param req[in] Request parsed by router_parse().
 * @return Complete HTTP response string (static, do not free).
 */
const char *router_dispatch(const struct http_request *req);

#endif /* ACCESS_POINT_ROUTER_H */
