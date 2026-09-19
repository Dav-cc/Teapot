#ifndef TEA_RESPONSE_H
#define TEA_RESPONSE_H

#include "../parser/http_parser.h"
#include "../core/event.h"
#include "../http/server.h"

typedef enum{
    ST_OK = 200,
    ST_BAD_REQUEST = 300,
    ST_TEAPOT = 418,
    ST_INTERNAL_SERVER_ERROR = 500 
} status;

typedef struct {
    status code;

    http_header headers[32];
    size_t headers_count;

    http_slice body;
    http_slice reason;
}http_response;

void http_response_set_reason(http_response* res,status code);
void http_response_add_header(http_response* res, const char* name , const char* value);
void http_response_set_body(http_response* res, const char* body);
void http_response_set_status(http_response* res,status code);
size_t http_response_serializer(http_response* res, dbuffer* write_buffer);
http_response* http_response_create();
void http_response_destroy();

#endif
