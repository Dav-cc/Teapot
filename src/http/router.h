#ifndef _ROUTEA_H_
#define _ROUTEA_H_

#include "sock.h"

typedef void* (*router_handler)(http_request* req); // this void* is http_response in future

typedef struct {
    http_method method;
    const char* path;
    router_handler rout_handler;
} router;

router* router_match(http_request* req);

void* root_handler(http_request* req);
void* dev_handler(http_request* req);
void* post_handler(http_request* req);


#endif
