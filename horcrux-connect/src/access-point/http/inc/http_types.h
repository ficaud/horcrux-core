#ifndef ACCESS_POINT_HTTP_TYPES_H
#define ACCESS_POINT_HTTP_TYPES_H

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

// ===========================================================================
// Definitions
// ===========================================================================
#define HTTP_RESPONSE_COUNT (20)

// ===========================================================================
// Public function declaration
// ===========================================================================
/**
 * @brief Pre-defined HTTP response indices.
 *
 * Each entry in @ref http_responses_list corresponds to one of these codes.
 * Use @ref HTTP_RESPONSE_COUNT_INVALID as a sentinel for internal errors.
 */
typedef enum
{
    HTTP_RESPONSE_OK = 0,
    HTTP_RESPONSE_CREATED,
    HTTP_RESPONSE_NO_CONTENT,
    HTTP_RESPONSE_MOVED_PERMANENTLY,
    HTTP_RESPONSE_FOUND,
    HTTP_RESPONSE_NOT_MODIFIED,
    HTTP_RESPONSE_BAD_REQUEST,
    HTTP_RESPONSE_UNAUTHORIZED,
    HTTP_RESPONSE_FORBIDDEN,
    HTTP_RESPONSE_NOT_FOUND,
    HTTP_RESPONSE_METHOD_NOT_ALLOWED,
    HTTP_RESPONSE_CONFLICT,
    HTTP_RESPONSE_PAYLOAD_TOO_LARGE,
    HTTP_RESPONSE_UNSUPPORTED_MEDIA_TYPE,
    HTTP_RESPONSE_UNPROCESSABLE_ENTITY,
    HTTP_RESPONSE_TOO_MANY_REQUESTS,
    HTTP_RESPONSE_INTERNAL_SERVER_ERROR,
    HTTP_RESPONSE_SERVICE_UNAVAILABLE,
    HTTP_RESPONSE_JSON_OK,
    HTTP_RESPONSE_COUNT_INVALID,
    HTTP_RESPONSE_MAX = HTTP_RESPONSE_COUNT,
} http_response_e;

/**
 * @brief Table of pre-built HTTP response strings.
 *
 * Indexed by @ref http_response_e. Each entry is a complete HTTP/1.1
 * response including status line, headers, and body.
 */
extern const char *http_responses_list[HTTP_RESPONSE_COUNT];

#ifdef __cplusplus
}
#endif

#endif /* ACCESS_POINT_HTTP_TYPES_H */
