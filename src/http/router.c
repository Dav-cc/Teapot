#include "router.h"
#include "../parser/http_parser.h"
#include <stdlib.h>

static router routers[] = {
    {HTTP_GET, "/", root_handler},
    {HTTP_GET, "/dev", dev_handler},
    {HTTP_GET, "/api/test", post_handler},
};

static size_t route_count = sizeof(routers) / sizeof(routers[0]);

router* find_router(http_request* req){
    for(size_t i = 0 ; i< route_count; i++){
        if(routers[i].method != req->mtd)
            continue;
        if((slice_eq_string(&req->path, (char*)routers[i].path)) != 0)
            continue;
        return &routers[i];
    }
    return NULL;
}

void* root_handler(http_request* req){
    return NULL;
}
void* dev_handler(http_request* req){
    return NULL;
}
void* post_handler(http_request* req){
    return NULL;
}
