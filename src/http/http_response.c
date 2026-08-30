#include "http_response.h"
#include "server.h"
#include "../core/log.h"
#include "../core/event.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

static bool http_slice_eq(http_slice_t s, const char *str){
    size_t len = strlen(str);
    return s.len == len && memcmp(s.data, str, len) == 0;
}

http_response_t* http_response_serializer(http_parser_t* p,http_request_t* req){
    char response[200];
    http_response_t* res = calloc(1,sizeof(*res));
    if(!res){
        log_message(LOG_LEVEL_ERROR, "error in allocating memory");
        return NULL;
    }
    if(http_slice_eq(req->method,"GET") == 0){
        if(http_slice_eq(req->path,"/dev") == 0){
            res->headers[0].key.data = "Server",
            res->headers[0].key.len = strlen(res->headers[0].key.data);
            res->headers[0].value.data = "TEAPOT/0.0.1";
            res->headers[0].value.len = strlen(res->headers[0].value.data);



            res->headers[1].key.data = "Content-Type";
            res->headers[1].key.len = strlen(res->headers[1].key.data);
            res->headers[1].value.data = "text/plain";
            res->headers[1].value.len = strlen(res->headers[1].value.data);
            

            res->body = "Hello From TEAPOT";
            res->body_len = strlen(res->body);

            // res->headers[2].key.data = "Content-Length",
            // res->headers[2].key.len = strlen(res->headers[2].key.data);
            // res->headers[2].value.len = p->content_length;
            res->body_len = strlen(res->body);

            res->headers_count = 2;
            res->reason = "OK";
            res->status = 200;
            return res;
        }
        res->status = 404;
        res->reason = "Not Found";
    }
    if(http_slice_eq(req->method,"POST") == 0){
        if(http_slice_eq(req->path, "/echo")){
            // TODO: 
        }
    }
        return res;
}

http_response_t* http_response_builder(http_parser_t*p, http_request_t* req, Connection* conn, EventLoop* loop){
    http_response_t *res = http_response_serializer(p, req);
    if (!res) {
      return NULL;
    }
    buffer_t *wb = conn->write_buff;

    wb->len = 0;
    wb->offset = 0;
    int n = snprintf(wb->data + wb->len, wb->cap - wb->len, "HTTP/1.1 %d %s\r\n",
                   res->status, res->reason);
    if (n < 0 || (size_t)n >= wb->cap - wb->len){
        return NULL;}

    wb->len += n;
    for (size_t i = 0; i < res->headers_count; i++) {
        n = snprintf(wb->data + wb->len, wb->cap - wb->len, "%.*s: %.*s\r\n",
                     (int)res->headers[i].key.len, res->headers[i].key.data,
                     (int)res->headers[i].value.len, res->headers[i].value.data);
        if (n < 0 || (size_t)n >= wb->cap - wb->len){
          return NULL;
        }
        wb->len += n;
    }
    n = snprintf(wb->data + wb->len, wb->cap - wb->len,
                 "Content-Length: %zu\r\n"
                 "\r\n",
                 res->body_len);

    if (n < 0 || (size_t)n >= wb->cap - wb->len)
      return NULL;

    wb->len += n;

    if (res->body && res->body_len > 0) {
        if (wb->cap - wb->len < res->body_len)
          return NULL;

        memcpy(wb->data + wb->len, res->body, res->body_len);
        wb->len += res->body_len;
    }
    log_message(LOG_LEVEL_INFO,"this buffer going for sending \n%.*s",conn->write_buff->len, conn->write_buff->data);
    eventloop_mod_event(loop, conn, EV_WRITABLE);
    return res;
}
