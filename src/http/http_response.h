#ifndef TEA_RESPONSE_H
#define TEA_RESPONSE_H

#include "../parser/http_parser.h"

typedef struct {
    int status;
    const char* reason;

    http_header_t headers[16];
    size_t headers_count;

    const char *body;
    size_t body_len;
}http_response_t;

#endif
