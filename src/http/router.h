#ifndef _ROUTEA_H_
#define _ROUTEA_H_

#include "sock.h"
#include "http_response.h"

typedef http_response* (*router_handler)(http_request* req); // this void* is http_response in future

typedef struct {
    http_method method;
    const char* path;
    router_handler rout_handler;
} router;

router* find_router(http_request* req);

http_response* root_handler(http_request* req);
http_response* dev_handler(http_request* req);
http_response* post_handler(http_request* req);


#endif
