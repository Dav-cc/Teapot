#ifndef __PARSER_TEAPOT_H
#define __PARSER_TEAPOT_H

#include "../core/rb.h"

#define MAX_HEADERS_COUNT 32U

typedef struct http_slice{
    char* key;
    size_t len;
}http_slice_t;

typedef struct http_headers{
    http_slice_t name;
    http_slice_t val;
}http_headers_t;

typedef enum{
    PARSER_DONE,
    PARSER_PARSING,
    PARSER_ERROR,
}http_parser_state_t;

typedef struct http_request{
    http_slice_t method;
    http_slice_t path;
    http_slice_t version;

    size_t headers_count;
    http_headers_t headers[MAX_HEADERS_COUNT];
    http_parser_state_t state;

    http_slice_t body;

    size_t content_len;
}http_request_t;


typedef struct http_parser{
    http_parser_state_t state;
    size_t content_len;
    size_t headers_count;
}http_parser_t;

int  parse_request_line(buffer_t* read_buffer, size_t read_len, http_request_t* req);
void parser_destroy(http_request_t* req);

#endif
