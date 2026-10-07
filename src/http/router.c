#include "router.h"
#include "../parser/http_parser.h"
#include "../static/static.h"
#include "http_response.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include "../static/static.h"

static router routers[] = {
    {HTTP_GET, "/", EXACT_ROUTE ,root_handler},
    {HTTP_GET, "/dev", EXACT_ROUTE,dev_handler},
    {HTTP_GET, "/static", PREFIX_ROUTE,static_handler},
    {HTTP_POST, "/api/test",EXACT_ROUTE, post_handler},
};

static size_t route_count = sizeof(routers) / sizeof(routers[0]);

router* find_router(http_request* req){
    for(size_t i = 0 ; i< route_count; i++){
        if(routers[i].method != req->mtd)
            continue;
        if(routers[i].type == EXACT_ROUTE){
            if((slice_eq_string(&req->path, (char*)routers[i].path)) != 0)
                continue;
        }
        if(routers[i].type == PREFIX_ROUTE){
            if(memcmp(req->path.ptr, "/static/", 8) != 0)
                continue;
        }
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
    http_response_add_header(res, "Connection", "keep-alive");
    http_response_set_body(res,body);
    return res;
}

http_response* dev_handler(http_request* req){
    http_response* res = http_response_create();
    if(!res){
        return NULL;
    }

    http_response_set_status(res, ST_NOT_IMPLEMENTED);
    http_response_set_reason(res, res->code);
    http_response_add_header(res, "Server", "Teapot-v0.0.1");
    http_response_add_header(res, "Connection", "keep-alive");
    http_response_set_body(res,"Not Implemented");
    return res;
}

http_response* post_handler(http_request* req){
    http_response* res = http_response_create();
    if(!res){
        return NULL;
    }

    http_response_set_status(res, ST_NOT_IMPLEMENTED);
    http_response_set_reason(res, res->code);
    http_response_add_header(res, "Server", "Teapot-v0.0.1");
    http_response_add_header(res, "Connection", "keep-alive");
    http_response_set_body(res,"Not Implemented");
    return res;
}

http_response* static_handler(http_request* req){
    http_response* res = http_response_create();
    if(!res){return NULL;}

    char* ver_path = path_verifyer(req);

    int filefd = open(ver_path, O_RDONLY);
    if(filefd == -1){
        if(errno == ENOENT)
            goto err_404;
    }

    file_state *st = get_file_state(filefd);
    if(!st)
        goto err_500;

    st->file_path = ver_path;

    st = find_mime_type(st, st->file_path);


err_404:
    http_response_set_status(res, ST_NOT_FOUND);
    http_response_set_reason(res, res->code);
    http_response_add_header(res, "Server", "Teapot-v0.0.1");
    http_response_add_header(res, "Connection", "keep-alive");
    http_response_set_body(res,"NO such file or dir");
    return res;

err_500:
    http_response_set_status(res, ST_INTERNAL_SERVER_ERROR);
    http_response_set_reason(res, res->code);
    http_response_add_header(res, "Server", "Teapot-v0.0.1");
    http_response_add_header(res, "Connection", "keep-alive");
    http_response_set_body(res,"Server Error");
    return res;
}
