#ifndef PARSER_TEAPOT_H
#define PARSER_TEAPOT_H
#include <stdlib.h>
#include "../core/dbuff.h"

typedef enum {
    PARSER_NEED_MORE = 0,
    PARSER_COMPLETE,
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
    HTTP_GET = 0,
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
    size_t consumed;    // we parse until conn->readbuffer->len == consumed if parsing was successfull returning with PARSER_NEED_MORE
    size_t headers_count;
    http_parser_result result;
}http_request;

typedef enum{
    NO_ERROR =1,
    ERROR_REQ_NOT_VALID = 0,
    ERROR_TOO_MANY_HEADERS = -1,  // max 32 headres
    ERROR_PARSER_NEED_MORE = -2,
}http_error_type;

typedef struct{
    int any_error;
    http_error_type type;
}http_error;

http_parser_result http_parser_parse(dbuffer* read_buf);
http_request* http_parser_request_line(http_request* req,dbuffer* read_buf);
http_request* http_parser_headers(http_request* req, dbuffer* read_buf);
http_request* http_parser_body(http_request* req, dbuffer* read_buf);
http_request* http_parser_handle_Get(http_request* req, dbuffer* read_buf);


ssize_t slice_eq_string(http_slice* slice, char* string);
#endif
