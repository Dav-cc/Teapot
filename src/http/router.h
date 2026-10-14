#ifndef _ROUTEA_H_
#define _ROUTEA_H_

#include "sock.h"
#include "http_response.h"
#include "../core/dbuff.h"

typedef http_response* (*router_handler)(http_request* req); // this void* is http_response in future

typedef enum{
    EXACT_ROUTE,
    PREFIX_ROUTE
} router_type;

typedef struct {
    http_method method;
    const char* path;
    router_type type;
    router_handler rout_handler;
} router;

typedef struct {
    int file_fd;
    ssize_t file_size;
    const char* mime_type;
    const char* file_path;
    dbuffer* write_buf;
} file_state;

router* find_router(http_request* req);

http_response* root_handler();
http_response* static_handler(http_request* req);
http_response* dev_handler(http_request* req);
http_response* post_handler(http_request* req);
http_response* unknown_path_handler();
http_response* error_handler();
void reset_file_stat(file_state *st);

#endif
