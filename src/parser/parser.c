#include "parser.h"
#include "../core/log.h"
#include <string.h>
// #include <errno.h>


int  parse_request(buffer_t* read_buffer, size_t read_len, http_request_t* req){
    int reqline_res = parse_request_line(read_buffer, read_len, req);
    if(reqline_res != 0){
        log_message(LOG_LEVEL_ERROR, "error in parsing request line");
    }
    int headers_res = parse_headers(read_buffer, read_len, req);
    if(headers_res != 0){
        log_message(LOG_LEVEL_ERROR, "error in parsing headers ");
    }
    int body = parse_body(read_buffer, read_len, req);
    if(body != 0){
        log_message(LOG_LEVEL_ERROR, "error in parsing body ");
    }
    return 0;
}

int parse_request_line(buffer_t* read_buffer, size_t read_len, http_request_t* req){
    char* c = read_buffer->data;
    char* first_space = strchr(c, ' ');
    if(!first_space){
        log_message(LOG_LEVEL_ERROR, "1)dont find spcae in buffer");
        return -1;
    }
    char* second_space = strchr(first_space + 1, ' ');
    if(!second_space){
        log_message(LOG_LEVEL_ERROR, "2)dont find spcae in buffer");
        return -1;
    }
    char* third_space = strstr(second_space + 1, "\r\n");
    if(!third_space){
        log_message(LOG_LEVEL_ERROR, "3)dont find spcae in buffer");
        return -1;
    }
    req->method.key = c;
    req->method.len = first_space - c;

    req->path.key = first_space + 1;
    req->path.len = second_space - req->path.key;

    req->version.key = second_space + 1;
    req->version.len = third_space - req->version.key;
    
    req->request_line_len = req->method.len + req->path.len + req->version.len + 4;  // +4 for \r\n

    log_message(LOG_LEVEL_INFO,"request line parsed || method: %.*s || path: %.*s || version: %.*s",(int)req->method.len, req->method.key,(int)req->path.len, req->path.key,(int)req->version.len, req->version.key);
    return 0;
}

int parse_headers(buffer_t *read_buffer, size_t read_len, http_request_t *req){
    log_message(LOG_LEVEL_INFO, " parsing headers started");
    int j = 0;
    char* s;
    char* c = read_buffer->data + req->request_line_len ; 

    while(1){
        if (c[0] == '\r' && c[1] == '\n'){
            req->body_start = (c + 2) - read_buffer->data;
            req->body.key = read_buffer->data + req->body_start;
            break;
        }

        char* ss = strchr(c , ':');
        if(!ss)
            return -1;
        char* ls = strstr(c, "\r\n");
        if(!ls)
            return -1;
        req->headers[j].name.key = c;
        req->headers[j].name.len = ss - c;

        char* val_start = ss + 1; // +1 for :
        while (*val_start == ' ') // spaece afeter : 
          val_start++;
        req->headers[j].val.key = val_start;
        req->headers[j].val.len = ls - val_start;

        j++;

        if(j >= MAX_HEADERS_COUNT){
            log_message(LOG_LEVEL_ERROR, "Too many errors");
            break;
        }

        c = ls + 2; // +2 for \r\n
        req->headers_count = j;
    }
    for(int k = 0; k < j;k++){
        req->headers_len += (req->headers[k].name.len+req->headers[k].val.len);
        log_message(LOG_LEVEL_INFO, "headers.name = %.*s <--> headers.val = %.*s", req->headers[k].name.len, req->headers[k].name.key, req->headers[k].val.len, req->headers[k].val.key);
    }
    return 0;
}

int parse_body(buffer_t *read_buffer, size_t read_len, http_request_t *req){
    char* body_content = read_buffer->data + req->request_line_len + req->headers_len +(req->headers_count * 4) + 2; // +4 bytes for \r\n +2 for last double \r\n
    log_message(LOG_LEVEL_INFO, "body=%s\nand it's length is = %d", body_content , strlen(body_content)); 
    return 0;
}

http_headers_t* get_header(http_request_t* req, char* dest){
    http_headers_t* hdr = req->headers ;
    for (int i = 0; i< req->headers_count; i++){
      if (req->headers[i].name.len == strlen(dest) &&strncmp(req->headers[i].name.key, dest, req->headers[i].name.len) ==0) {
        return &req->headers[i];
      }
    }
    return NULL;
}
