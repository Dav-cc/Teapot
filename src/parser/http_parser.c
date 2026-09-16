#include <stddef.h>
#define _GNU_SOURCE
#include "http_parser.h"
// #include "../http/server.h"
#include "../core/log.h"
#include <string.h>
#include <stdlib.h>

http_error err = {
    .any_error = 0,
    .type = NO_ERROR,
};

int slice_eq_string(http_slice* slice, char* string){
    size_t str_len = strlen(string);
    if(slice->len != str_len)
        return 0;
    return memcmp(slice->ptr, string, str_len) == 0;
}



char* find_slice(char* src, char* dest, size_t dest_size){
    // TODO : dont hardcode this  .. . . 
    char* d = (char*)memmem(src, 1024 ,dest , dest_size);
    if(!d){
        return NULL;
    }
    return d;
}



http_header* header_lookup(http_header* headers, const char* name, size_t headers_count){
    int name_len = strlen(name);
    for(int i = 0; i <headers_count; i++ ){
        if (slice_eq_string(&headers[i].name, name) )
            return &headers[i];
        }
    return NULL;
}

int slice_lookup(http_slice* slice, const char* name){
    int j = strncmp(slice->ptr, name, strlen(name));
    if(j == 0)
        return 0;
    return -1;
}

size_t slice_to_ul(http_header* con_len){
    size_t u_len;
    char tmp[32] = {0};

    char* ptr = memcpy(tmp, con_len->value.ptr, con_len->value.len);
    if(!ptr){
        return -1;
    }
    tmp[con_len->value.len] = '\0';
    u_len = strtoul(tmp, NULL, 10);
    return u_len;
}

http_parser_result http_parser_parse(dbuffer* buf){
    http_request* req = calloc(1, sizeof(http_request));
    req = http_parser_request_line(req, buf);
    if(err.any_error){
        switch (err.type) {
            case NO_ERROR:
                break;

            case ERROR_PARSER_NEED_MORE:
                return PARSER_NEED_MORE;

            case ERROR_REQ_NOT_VALID:
            case ERROR_TOO_MANY_HEADERS:
                log_message(LOG_LEVEL_ERROR, "error in parsing reqline : %s", err.txt);
                return PARSER_ERROR;
        }
    }
    //TODO: SETUP HEADERS PARSING
    
    req = http_parser_headers(req, buf);
    if(err.any_error){
        switch (err.type) {
            case NO_ERROR:
                break;

            case ERROR_PARSER_NEED_MORE:
                return PARSER_NEED_MORE;

            case ERROR_REQ_NOT_VALID:
            case ERROR_TOO_MANY_HEADERS:
                log_message(LOG_LEVEL_ERROR, "error in parsing reqline : %s", err.txt);
                return PARSER_ERROR;
        }
    }
    req = http_parser_body(req, buf);
    // if(err.any_error){
    //     log_message(LOG_LEVEL_ERROR, "error in parsing body : %s", err.txt);
    //     return PARSER_ERROR;
    // }
    return PARSER_ERROR;
}


http_request* http_parser_request_line(http_request* req,dbuffer* read_buf){
    char* start = read_buf->data;
    char *check = find_slice(read_buf->data, "\r\n", 2);
    if(!check){
        err.any_error = 1;
        err.type = ERROR_PARSER_NEED_MORE ;
        log_message(LOG_LEVEL_ERROR, "couldn't find crlf in req");
        return NULL;
    }


    char* space1 = find_slice(read_buf->data, " ", 1);
    if(!space1){
        err.any_error = 1;
        err.type = ERROR_REQ_NOT_VALID;
        log_message(LOG_LEVEL_ERROR, "couldn't find 1-space in req");
        return NULL;
    }


    char* space2 = find_slice(space1+ 1, " ", 1);
    if(!space2){
        err.any_error = 1;
        err.type = ERROR_REQ_NOT_VALID;
        log_message(LOG_LEVEL_ERROR, "couldn't find 2-space in req");
        return NULL;
    }

    req->method.ptr = start;
    req->method.len = space1 - start;

    req->path.ptr = space1 + 1;
    req->path.len = space2 - (space1 + 1);

    req->version.ptr = space2 + 1;
    req->version.len = check - (space2 + 1);

    err.any_error = 0;
    err.type = NO_ERROR;

    read_buf->offset += req->method.len + req->path.len + req->version.len + 1 + 1 + 2 ; // +1's for spaces and +2 for \r\n

    return req;
}

http_request* http_parser_headers(http_request* req, dbuffer* read_buf){
    size_t h_offset = 0, i = 0;
    char* h_start = read_buf->data + read_buf->offset;
    char* h_end = find_slice(h_start, "\r\n\r\n", 4);
    // if(!h_end){
    //     log_message(LOG_LEVEL_ERROR, "couldn't find end of header");
    //     err.any_error = 1;
    //     err.type = ERROR_PARSER_NEED_MORE;
    //     return req;
    // }
    // while(1){
    //     char* crlf = find_slice(h_start + h_offset, "\r\n", 2);
    //     if(!crlf){
    //         log_message(LOG_LEVEL_ERROR, "not valid header formate");
    //         err.any_error = 1;
    //         err.type = ERROR_REQ_NOT_VALID;
    //         return req;
    //     }

    //     char* col = find_slice(h_start + h_offset, ":", 1);
    //     if(!col){
    //         log_message(LOG_LEVEL_ERROR, "not valid header formate");
    //         err.any_error = 1;
    //         err.type = ERROR_REQ_NOT_VALID;
    //         return req;
    //     }
    //     req->headers[i].name.ptr = h_start + h_offset;
    //     req->headers[i].name.len = col - h_start;
    
    
    //     req->headers[i].value.ptr = col + 1 + 1; // +1 for space after col
    //     req->headers[i].value.len = crlf - (col + 2);

    //     h_offset += req->headers[i].name.len +req->headers[i].value.len + 1 +
    //     2; // +1 for : and +2 for \r\n h_start += h_offset; i++;
    // }
    char *cur = read_buf->data + read_buf->offset;

    while (cur < h_end) {

      char *crlf = find_slice(cur, "\r\n", 2);
      char *col = find_slice(cur, ":", 1);

      if (!crlf || !col || col > crlf){
        log_message(LOG_LEVEL_ERROR, "not valid header formate");
        err.any_error = 1;
        err.type = ERROR_REQ_NOT_VALID;
        return req;
      }

      req->headers[i].name.ptr = cur;
      req->headers[i].name.len = col - cur;

      req->headers[i].value.ptr = col + 2;
      req->headers[i].value.len = crlf - (col + 2);

      cur = crlf + 2;
      i++;
      req->headers_count++;
    }
    read_buf->offset = (h_end - read_buf->data) + 4;
    if(err.any_error == 0){        
        err.type=  NO_ERROR;
        return req;
    }
    err.type = ERROR_REQ_NOT_VALID;
    return req;
}

http_request* http_parser_body(http_request* req, dbuffer* read_buf){
    char tmp[32];
    char* start_body = read_buf->data + read_buf->offset;
    size_t recived_body = read_buf->len - read_buf->offset;

    http_header *con_len = header_lookup(req->headers, "Content-Length", req->headers_count);
    if (!con_len) {
      log_message(LOG_LEVEL_ERROR, "req is POST but no content length");
      err.any_error = 1;
      err.type = ERROR_REQ_NOT_VALID;
      return req;
    }
    size_t body_len = slice_to_ul(con_len); 
    if(body_len == -1){
      log_message(LOG_LEVEL_ERROR, "body message length invalid");
      err.any_error = 1;
      err.type = ERROR_REQ_NOT_VALID;
      return req;
    }
    if(body_len > recived_body){
        err.any_error =1;
        err.type = ERROR_PARSER_NEED_MORE;
        return req;
    }
    req->body.ptr = start_body;
    req->body.len = recived_body;
    log_message(LOG_LEVEL_INFO, "body parsed");
    log_message(LOG_LEVEL_INFO, "body : %.*s", req->body.len, req->body.ptr);
    return req;

}
