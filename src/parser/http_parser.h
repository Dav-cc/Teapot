#ifndef TEAPOT_PARSER_H
#define TEAPOT_PARSER_H

#include "../core/log.h"
#include "../core/dbuff.h"
#include <stddef.h>

typedef enum {
    PARSER_RESULT_OK = 0, // http request parsed succesfully
    PARSER_RESULT_NEED_MORE = 1, // request is not complitly recived
    PARSER_RESULT_ERROR = 2,      // error in parsing
}http_parser_result_t;

typedef enum{
    PARSER_STATE_REQUEST_LINE = 0,
    PARSER_STATE_HEADERS = 1,
    // PARSER_STATE_BODY_CONTENT_LENGTH = 2,
    PARSER_STATE_BODY_PARSING = 3,
    PARSER_COMPLETE = 4,
    PARSER_STATE_ERROR = 5,
}http_parser_state_t;

typedef enum {
    HTTP_ERR_NONE                 = 0,
    HTTP_ERR_INVALID_METHOD       = 1,
    HTTP_ERR_INVALID_REQUEST_TARGET = 2,
    HTTP_ERR_INVALID_VERSION      = 3,
    HTTP_ERR_HEADER_NAME_TOO_LONG = 4,
    HTTP_ERR_HEADER_VALUE_TOO_LONG = 5,
    HTTP_ERR_TOO_MANY_HEADERS     = 6,
    HTTP_ERR_INVALID_CONTENT_LENGTH = 7,
    HTTP_ERR_BODY_TOO_LARGE       = 8,
    HTTP_ERR_INCOMPLETE           = 9,
} http_parse_error_e;

typedef struct {
    http_parse_error_e code;
    size_t offset;       // where error occurred
    const char* messg;
} http_parser_error_t;

typedef struct {
    char* data ; // pointer to data
    size_t len;     // words length
}http_slice_t;

typedef struct {
    http_slice_t key;
    http_slice_t value;
}http_header_t;

typedef struct{
    http_slice_t method;
    http_slice_t path;
    http_slice_t version;

    http_header_t headers [256];
    size_t headers_count;

    http_slice_t body;
    size_t content_length;
    int keep_alive;
    size_t header_bytes_parsed;
    size_t body_bytes_parsed;
} http_request_t;

typedef struct {
    http_parser_state_t state;
    http_parser_error_t error;
    size_t bytes_consumed;
    size_t content_length;
    size_t headers_consumed;

    http_request_t request;  // building request
}http_parser_t;

http_parser_result_t http_parser_parse(http_parser_t* p, dbuffer* buf, size_t len, size_t* consumed);
http_parser_t* http_parser_init();
#endif
