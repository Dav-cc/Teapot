#include "http_parser.h"
#include <stddef.h>
#include <string.h>
#include <stdbool.h>

static bool http_parser_parse_req_line(http_parser_t* p, buffer_t*b , size_t len){
    // TODO:
}
static http_parser_result_t http_parser_parse_headers(http_parser_t* p, buffer_t*b , size_t len){
    // TODO:
}
static http_parser_result_t http_parser_parse_body(http_parser_t* p, buffer_t*b , size_t len){
    // TODO:
}
static http_parser_result_t http_parser_get_content_length(http_parser_t* p, buffer_t*b , size_t len){
    // TODO:
}
static http_parser_result_t http_parser_get_header(http_parser_t* p, buffer_t*b , size_t len, const char* header_value){
    // TODO:
}

http_parser_result_t http_parser_parse(http_parser_t* p, buffer_t* buf, size_t len, size_t* consumed){
    size_t start = p->bytes_consumed;
    while(p->bytes_consumed - start < len){
        switch(p->state){
            case PARSER_STATE_REQUEST_LINE:
                if(!http_parser_parse_req_line(p, buf, len))
                    return PARSER_RESULT_NEED_MORE;
                    break; 
            case PARSER_STATE_HEADERS :
                if(!http_parser_parse_headers(p, buf, len))
                    return PARSER_RESULT_NEED_MORE;
                    break; 
            case PARSER_STATE_BODY_CONTENT_LENGTH: 
                if(!http_parser_get_content_length(p, buf, len))
                    return PARSER_RESULT_NEED_MORE;
                    break; 
            case PARSER_STATE_BODY_PARSING:
                if(!http_parser_parse_body(p, buf, len))
                    return PARSER_RESULT_NEED_MORE;
                    break; 
            case PARSER_COMPLETE:
                *consumed = p->bytes_consumed -start;
                return PARSER_RESULT_OK;
                    break; 
            case PARSER_STATE_ERROR:
                *consumed = p->bytes_consumed -start;
                log_message(LOG_LEVEL_ERROR, "we have error in parsing");
                return PARSER_RESULT_ERROR;
                    break; 
        }
    }
    return PARSER_RESULT_ERROR;
}
