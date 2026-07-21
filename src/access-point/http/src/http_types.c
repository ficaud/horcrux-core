/**
 * @file http_types.c
 *
 * @brief HTTP response table definition for the captive portal.
 *
 * @author Julien F.
 * @date 2026-07-13
 *
 * @details Defines the pre-built HTTP response strings used by the
 *          HTTP server and handlers.
 */

#include "http_types.h"

// ===========================================================================
// Variables definition
// ===========================================================================
const char *http_responses_list[HTTP_RESPONSE_COUNT] = {
    [HTTP_RESPONSE_OK] = "HTTP/1.1 200 OK\r\n"
                         "Content-Length: 2\r\n"
                         "Connection: close\r\n"
                         "\r\n"
                         "OK",
    [HTTP_RESPONSE_CREATED] = "HTTP/1.1 201 Created\r\n"
                              "Content-Length: 7\r\n"
                              "Connection: close\r\n"
                              "\r\n"
                              "Created",
    [HTTP_RESPONSE_NO_CONTENT] = "HTTP/1.1 204 No Content\r\n"
                                 "Content-Length: 0\r\n"
                                 "Connection: close\r\n"
                                 "\r\n",
    [HTTP_RESPONSE_MOVED_PERMANENTLY] = "HTTP/1.1 301 Moved Permanently\r\n"
                                        "Content-Length: 0\r\n"
                                        "Connection: close\r\n"
                                        "\r\n",
    [HTTP_RESPONSE_FOUND] = "HTTP/1.1 302 Found\r\n"
                            "Content-Length: 0\r\n"
                            "Connection: close\r\n"
                            "\r\n",
    [HTTP_RESPONSE_NOT_MODIFIED] = "HTTP/1.1 304 Not Modified\r\n"
                                   "Content-Length: 0\r\n"
                                   "Connection: close\r\n"
                                   "\r\n",
    [HTTP_RESPONSE_BAD_REQUEST] = "HTTP/1.1 400 Bad Request\r\n"
                                  "Content-Type: text/plain\r\n"
                                  "Content-Length: 11\r\n"
                                  "Connection: close\r\n"
                                  "\r\n"
                                  "Bad Request",
    [HTTP_RESPONSE_UNAUTHORIZED] = "HTTP/1.1 401 Unauthorized\r\n"
                                   "Content-Type: text/plain\r\n"
                                   "Content-Length: 12\r\n"
                                   "Connection: close\r\n"
                                   "\r\n"
                                   "Unauthorized",
    [HTTP_RESPONSE_FORBIDDEN] = "HTTP/1.1 403 Forbidden\r\n"
                                "Content-Type: text/plain\r\n"
                                "Content-Length: 9\r\n"
                                "Connection: close\r\n"
                                "\r\n"
                                "Forbidden",
    [HTTP_RESPONSE_NOT_FOUND] = "HTTP/1.1 404 Not Found\r\n"
                                "Content-Type: text/plain\r\n"
                                "Content-Length: 9\r\n"
                                "Connection: close\r\n"
                                "\r\n"
                                "Not Found",
    [HTTP_RESPONSE_METHOD_NOT_ALLOWED] = "HTTP/1.1 405 Method Not Allowed\r\n"
                                         "Content-Type: text/plain\r\n"
                                         "Content-Length: 17\r\n"
                                         "Connection: close\r\n"
                                         "\r\n"
                                         "Method Not Allowed",
    [HTTP_RESPONSE_CONFLICT] = "HTTP/1.1 409 Conflict\r\n"
                               "Content-Type: text/plain\r\n"
                               "Content-Length: 8\r\n"
                               "Connection: close\r\n"
                               "\r\n"
                               "Conflict",
    [HTTP_RESPONSE_PAYLOAD_TOO_LARGE] = "HTTP/1.1 413 Payload Too Large\r\n"
                                        "Content-Type: text/plain\r\n"
                                        "Content-Length: 18\r\n"
                                        "Connection: close\r\n"
                                        "\r\n"
                                        "Payload Too Large",
    [HTTP_RESPONSE_UNSUPPORTED_MEDIA_TYPE] = "HTTP/1.1 415 Unsupported Media Type\r\n"
                                             "Content-Type: text/plain\r\n"
                                             "Content-Length: 22\r\n"
                                             "Connection: close\r\n"
                                             "\r\n"
                                             "Unsupported Media Type",
    [HTTP_RESPONSE_UNPROCESSABLE_ENTITY] = "HTTP/1.1 422 Unprocessable Entity\r\n"
                                           "Content-Type: text/plain\r\n"
                                           "Content-Length: 21\r\n"
                                           "Connection: close\r\n"
                                           "\r\n"
                                           "Unprocessable Entity",
    [HTTP_RESPONSE_TOO_MANY_REQUESTS] = "HTTP/1.1 429 Too Many Requests\r\n"
                                        "Content-Type: text/plain\r\n"
                                        "Content-Length: 16\r\n"
                                        "Connection: close\r\n"
                                        "\r\n"
                                        "Too Many Requests",
    [HTTP_RESPONSE_INTERNAL_SERVER_ERROR] = "HTTP/1.1 500 Internal Server Error\r\n"
                                            "Content-Type: text/plain\r\n"
                                            "Content-Length: 21\r\n"
                                            "Connection: close\r\n"
                                            "\r\n"
                                            "Internal Server Error",
    [HTTP_RESPONSE_SERVICE_UNAVAILABLE] = "HTTP/1.1 503 Service Unavailable\r\n"
                                          "Content-Type: text/plain\r\n"
                                          "Content-Length: 19\r\n"
                                          "Connection: close\r\n"
                                          "\r\n"
                                          "Service Unavailable",
    [HTTP_RESPONSE_JSON_OK] = "HTTP/1.1 200 OK\r\n"
                              "Content-Type: application/json\r\n"
                              "Content-Length: %zu\r\n"
                              "Connection: close\r\n"
                              "\r\n"
                              "%s",
    [HTTP_RESPONSE_COUNT_INVALID] = "HTTP/1.1 500 Internal Server Error\r\n"
                                    "Content-Type: text/plain\r\n"
                                    "Content-Length: 21\r\n"
                                    "Connection: close\r\n"
                                    "\r\n"
                                    "Invalid response index",
};
