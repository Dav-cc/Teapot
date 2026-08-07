#include "parser.h"
#include "../core/log.h"
#include <string.h>
// #include <errno.h>

int parse_request_line(buffer_t* read_buffer, size_t read_len, http_request_t* req){
    char* c = read_buffer->data;
    char* first_space = strchr(c, ' ');
    if(!first_space){
        log_message(LOG_LEVEL_ERROR, "1)dont find spcae in buffer");
        return NULL;
    }
    char* second_space = strchr(first_space, ' ');
    if(!second_space){
        log_message(LOG_LEVEL_ERROR, "2)dont find spcae in buffer");
        return NULL;
    }
    char* third_space = strstr(second_space, "\r\n");
    if(!third_space){
        log_message(LOG_LEVEL_ERROR, "3)dont find spcae in buffer");
        return NULL;
    }
    req->method.key = first_space;
    req->method.len = first_space - c;

    req->path.key = second_space;
    req->path.len = second_space - req->path.key;

    req->version.key = third_space;
    req->version.len = third_space - req->version.key;


    log_message(LOG_LEVEL_INFO, "request line parsed:\n method: %s, path: %s, version : %s", req->method.key, req->path.key,req->version.key);
    return 0;
}
