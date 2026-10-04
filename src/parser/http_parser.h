#ifndef PARSER_TEAPOT_H
#define PARSER_TEAPOT_H
#include <stdlib.h>
#include "../core/dbuff.h"


typedef enum {
    PARSER_STATE_REQLINE = 0,
    PARSER_STATE_HEADERS,
    PARSER_STATE_BODY,
    PARSER_STATE_DONE,
    PARSER_STATE_ERROR
}http_parser_state;

typedef enum {
    ERR_INVALID_REQ_LINE = 10,
    ERR_INVALID_HEADER_CONTENT,
    ERR_INVALLID_CONTENT_LENGTH,
    ERR_INVALID_HEADER_COUNT,
    ERR_REQUEST_NOT_COMPELETE,
    ERR_PARSER,
}http_err_type;

typedef struct{
    int any_err;
    http_err_type type;
}http_parser_error;

typedef enum {
    PARSER_NEED_MORE = 20,
    PARSER_OK,
    PARSER_ERROR
}http_parser_result;

typedef struct {
    char* ptr;
    size_t len;
}http_slice;

typedef struct {
    http_slice name;
    http_slice value;
} http_header;

typedef enum {
    HTTP_GET = 30,
    HTTP_POST,
    HTTP_UNKNOWN
} http_method;

typedef struct {
    http_method mtd;
    http_slice method;
    http_slice path;
    http_slice version;
    http_header headers[32];
    http_slice body;
    int keep_alive;   
    size_t headers_count;
    http_parser_result result;
    http_parser_state state;
    http_parser_error err;
}http_request;

// typedef enum{
//     NO_ERROR =40,
//     ERROR_REQ_NOT_VALID,
//     ERROR_TOO_MANY_HEADERS,  // max 32 headres
//     ERROR_PARSER_NEED_MORE,
// }http_error_type;

typedef struct{
    int any_error;
    http_err_type type;
}http_error;

http_parser_result http_parser_parse(void* conn,dbuffer* read_buf);
http_parser_error http_parser_request_line(http_request* req,dbuffer* read_buf);
http_parser_error http_parser_headers(http_request* req, dbuffer* read_buf);
http_parser_error http_parser_body(http_request* req, dbuffer* read_buf);
http_parser_error http_parser_handle_Get(http_request* req, dbuffer* read_buf);

void http_request_destroy(http_request* req);

int slice_eq_string(http_slice* slice, char* string);
#endif
