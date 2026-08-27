#include "http_response.h"
#include "server.h"
#include "../core/log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

static bool http_slice_eq(http_slice_t s, const char *str){
    size_t len = strlen(str);
    return s.len == len && memcmp(s.data, str, len) == 0;
}

http_response_t* http_response_builder(Connection* conn, http_request_t* req){
    char response[200];
    http_response_t* res = calloc(1,sizeof(*res));
    if(!res){
        log_message(LOG_LEVEL_ERROR, "error in allocating memory");
        return NULL;
    }
    if(http_slice_eq(req->method,"GET") == 0){
        if(http_slice_eq(req->path,"/dev") == 0){
            res->headers[0].key.data = "Server",
            res->headers[0].key.len = strlen(res->headers[1].key.data);
            res->headers[0].value.data = "TEAPOT/0.0.1";
            res->headers[0].value.len = strlen(res->headers[1].value.data);



            res->headers[1].key.data = "Content-Type";
            res->headers[1].key.len = strlen(res->headers[2].key.data);
            res->headers[1].value.data = "text/plain";
            res->headers[1].value.len = strlen(res->headers[2].value.data);
            

            res->body = "Hello From TEAPOT";

            res->headers[2].key.data = "Content-Length",
            res->headers[2].key.len = strlen(res->headers[0].key.data);
            res->headers[2].value.len = strlen(res->body);
            res->body_len = strlen(res->body);

            res->headers_count = 3;
            res->reason = "OK";
            res->status = 200;
        }
    }
    if(http_slice_eq(req->method,"POST") == 0){
        if(http_slice_eq(req->path, "/echo")){
            // TODO: 
        }
    }

}

