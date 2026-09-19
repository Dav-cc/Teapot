#include "http_response.h"
#include "../core/dbuff.h"
#include "../core/log.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

http_response* http_response_create(){
    http_response *res = calloc(1, sizeof(http_response));
    if(!res){
        return NULL;
    }
    return res;
};

void http_response_set_status(http_response* res,status code){
    switch (code) {
        case ST_BAD_REQUEST:
            res->code = 300;
            break;
        case ST_TEAPOT:
            res->code = 300;
            break;
        case ST_OK:
            res->code = 200;
            break;
        case ST_INTERNAL_SERVER_ERROR:
            res->code = 500;
            break;
    
    }
}

void http_response_set_reason(http_response* res,status code){
    switch (res->code) {
        case ST_BAD_REQUEST:
            res->reason.ptr = "Bad Request";
            res->reason.len = strlen(res->reason.ptr);
            break;
        case ST_TEAPOT:
            res->reason.ptr = "Ima Teapot";
            res->reason.len = strlen(res->reason.ptr);
            break;
        case ST_OK:
            res->reason.ptr = "OK";
            res->reason.len = strlen(res->reason.ptr);
            break;
        case ST_INTERNAL_SERVER_ERROR:
            res->reason.ptr = "Internal Server Error";
            res->reason.len = strlen(res->reason.ptr);
            break;
    }
}

void http_response_add_header(http_response* res, const char* name , const char* value){
    if(res->headers_count >= 32)
        return ;
    http_header* header = &res->headers[res->headers_count];
    header->name.ptr = (char*)name;
    header->name.len = strlen(name);

    header->value.ptr = (char*)value;
    header->value.len = strlen(value);

    res->headers_count++;
}

void http_response_set_body(http_response* res, const char* body){
    res->body.ptr = (char*)body;
    res->body.len = strlen(body);
};

size_t http_response_serializer(http_response* res, dbuffer *write_buffer){
    char con_len[5];
    char tmp[1024] = {0};
    int a = sprintf(tmp, "HTTP/1.1 %d %s\r\n", res->code, res->reason.ptr);
    dbuff_append(write_buffer, tmp, a);

    snprintf(con_len, sizeof(con_len), "%zu", res->body.len);
    http_response_add_header(res, "Content-Length", con_len);

    for(int i = 0; i< res->headers_count; i++){
       a = sprintf(tmp,"%s: %s\r\n", res->headers[i].name.ptr , res->headers[i].value.ptr);
        dbuff_append(write_buffer, tmp, a);
    }


    a = sprintf(tmp, "\r\n");
    dbuff_append(write_buffer, tmp, a);

    a = sprintf(tmp, "%s", res->body.ptr);
    dbuff_append(write_buffer, tmp, a);

    log_message(LOG_LEVEL_INFO, "%.*s", write_buffer->len, write_buffer->data);
    return 1;
}
