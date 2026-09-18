#ifndef TEA_RESPONSE_H
#define TEA_RESPONSE_H

#include "../parser/http_parser.h"
#include "../core/event.h"
#include "../http/server.h"

typedef enum{
    OK = 200,
    BAD_REQUEST = 400,
    TEAPOT = 418,
    INTERNAL_SERVER_ERROR =500
} status;

typedef struct {
    status code;

    http_header headers[32];
    size_t headers_count;

    http_slice body;
    http_slice reason;
}http_response;


http_response* http_response_serializer();
http_response* http_response_builder();

#endif
