#include "http_parser.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdbool.h>


static char* find_crlf(char* ch, size_t len){
    int i = 0;
    while(i + 1 < len){
        if(ch[i] == '\r' && ch[i+1] == '\n')
            return &ch[i];
        i++;
    }
    return NULL;
}

static bool http_parser_parse_req_line(http_parser_t* p, buffer_t* buf, size_t len){
    char *start = buf->data;
    char *end = find_crlf(start, len);
    if(!end){
        log_message(LOG_LEVEL_ERROR, "Not valid request line : couldn't find crlf in buffer");
        return false;
    }
    char* space1 = memchr(start, ' ', end - start);
    if(!space1){
        log_message(LOG_LEVEL_ERROR, "Not valid request line : couldn't find spaces in request line");
        return false;
    }
    char* space2 = memchr(space1+1, ' ', end - (space1+1));
    if(!space2){
        log_message(LOG_LEVEL_ERROR, "Not valid request line : couldn't find spaces in request line");
        return false;
    }
    p->request.method.data = start;
    p->request.method.len = space1 - start;

    p->request.path.data = space1 + 1;
    p->request.path.len = space2 - (space1+1);

    p->request.version.data = space2 + 1;
    p->request.version.len = end - (space2 - 1);

    p->state = PARSER_STATE_HEADERS;
    p->bytes_consumed = ((end+1) - start) + 1; // +1 for \n in crlf
    buf->data += p->bytes_consumed;
    return true;
}

static bool http_parser_parse_headers(http_parser_t* p, buffer_t* buf, size_t len){
    char* start = buf->data;
    while(p->bytes_consumed <= len){
        char* crlf = find_crlf(start, len - (start - buf->data));
        if(!crlf){
            log_message(LOG_LEVEL_ERROR, "Not valid headers in request, no crlf");
            return false;
        }
        if(crlf == start) break;
        char* colon = memchr(start, ':', crlf - start);
        if(!colon){
            log_message(LOG_LEVEL_ERROR, "Not valid headers in request, no colon");
            return false;
        }
        p->request.headers[p->request.headers_count].key.data = start;
        p->request.headers[p->request.headers_count].key.len = colon - start;

        char* value = colon+1;
        while (*value == ' ' || *value == '\t') {
            value++;
        }
        p->request.headers[p->request.headers_count].value.data = value ;
        p->request.headers[p->request.headers_count].value.len = crlf - p->request.headers[p->request.headers_count].value.data ;
        p->request.headers_count++;
        p->headers_consumed++;
        p->bytes_consumed = (crlf+2) - buf->data;
        start = crlf + 2;
    }
    p->state = PARSER_STATE_BODY_CONTENT_LENGTH;
    buf->data +=2; // for crlf
    buf->data += p->bytes_consumed;
    return true;
}

static http_header_t* http_parser_get_content_length(http_parser_t* p){
    for(size_t i = 0; i < p->request.headers_count; i++) {
        http_slice_t* key = &p->request.headers[i].key;
        if(key->len == strlen("Content-Length") &&
           strncasecmp(key->data,"Content-Length",key->len) == 0){
            p->state = PARSER_STATE_BODY_PARSING;
            return &p->request.headers[i];
        }
    }
    return NULL;
}

static bool http_parser_parse_body(http_parser_t* p, buffer_t* buf, size_t len){
    http_header_t* con_len = http_parser_get_content_length(p);
    if(!con_len){
        log_message(LOG_LEVEL_ERROR, "No content-length header in parsed headers");
        return false;
    }
    p->content_length = atoi(con_len->value.data);

    char* body_content = buf->data;
    p->state = PARSER_COMPLETE;
    return true;
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
                if(!http_parser_get_content_length(p))
                    return PARSER_RESULT_NEED_MORE;
                    break; 
            case PARSER_STATE_BODY_PARSING:
                if(!http_parser_parse_body(p, buf, len))
                    return PARSER_RESULT_NEED_MORE;
                    break; 
            case PARSER_COMPLETE:
                *consumed = p->bytes_consumed - start;
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
