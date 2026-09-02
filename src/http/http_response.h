#ifndef TEA_RESPONSE_H
#define TEA_RESPONSE_H

#include "../parser/http_parser.h"
#include "../core/event.h"
#include "../http/server.h"

typedef struct {
    int status;
    const char* reason;

    http_header_t headers[16];
    size_t headers_count;

    const char *body;
    size_t body_len;
}http_response_t;


http_response_t* http_response_serializer(http_parser_t* p,http_request_t* req);
http_response_t* http_response_builder(http_parser_t* p, http_request_t* req,Connection* conn, EventLoop* loop);

#endif
