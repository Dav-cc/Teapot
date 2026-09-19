#include "router.h"
#include "../parser/http_parser.h"
#include "http_response.h"
#include <stdlib.h>
#include <string.h>

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
        if((slice_eq_string(&req->path, (char*)routers[i].path)) == 0)
            continue;
        return &routers[i];
    }
    return NULL;
}

http_response* root_handler(http_request* req){
    http_response* res = http_response_create();
    if(!res){
        return NULL;
    }

    char* server_name = "Teapot-v0.0.1";
    const char *body = "Hello From TEAPOT";
 
    http_response_set_status(res, ST_OK);
    http_response_set_reason(res, res->code);
    http_response_add_header(res, "Server", server_name);
    http_response_set_body(res,body);
    return res;
}



http_response* dev_handler(http_request* req){
    return NULL;
}



http_response* post_handler(http_request* req){
    return NULL;
}
