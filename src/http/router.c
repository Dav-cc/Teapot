#include "router.h"
#include "../parser/http_parser.h"
#include "../static/static.h"
#include "http_response.h"
#include <stddef.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include "../static/static.h"

static router routers[] = {
    {HTTP_GET, "/", EXACT_ROUTE ,root_handler},
    {HTTP_GET, "/static", PREFIX_ROUTE,static_handler},
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
            size_t prefix_len = strlen(routers[i].path);

            if (req->path.len < prefix_len)
              continue;
            if(memcmp(req->path.ptr, routers[i].path, prefix_len) != 0)
                continue;
          }
        return &routers[i];
    }
        return NULL;
}

http_response* unknown_path_handler(){
    http_response* res = http_response_create();
    if(!res){
        return NULL;
    }

    char* server_name = "Teapot-v0.0.1";
    const char *body = "<html><h1>Not implemented </h1></html>";
 
    http_response_set_status(res, ST_NOT_IMPLEMENTED);
    http_response_set_reason(res);
    http_response_add_header(res, "Server", server_name);
    http_response_add_header(res, "Connection", "keep-alive");
    http_response_set_body(res,body);
    return res;
}

http_response* root_handler(){
    http_response* res = http_response_create();
    if(!res){
        return NULL;
    }

    char* server_name = "Teapot-v0.0.1";
    const char *body = "Hello From TEAPOT";
 
    http_response_set_status(res, ST_OK);
    http_response_set_reason(res);
    http_response_add_header(res, "Server", server_name);
    http_response_add_header(res, "Connection", "keep-alive");
    http_response_set_body(res,body);
    return res;
}

http_response* static_handler(http_request* req){
    const char* err_body;
    char filesize[32];
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

    res->file_stat = st;
    st->file_path = ver_path;

    st = find_mime_type(st, st->file_path);

    st = set_body_response(st);

    snprintf(filesize, sizeof(filesize), "%zu", st->file_size);

    res->body.ptr = st->write_buf->data;

    res->body.len = st->write_buf->len;

    res->con_len = st->file_size;


    http_response_set_status(res, ST_OK);
    http_response_set_reason(res);
    http_response_add_header(res, "Server", "Teapot-v0.0.1");
    http_response_add_header(res, "Connection", "keep-alive");
    http_response_add_header(res, "Content-Type", st->mime_type);


    reset_file_stat(st);
    return res;

err_404:
    err_body = "NO such file or dir";

    http_response_set_status(res, ST_NOT_FOUND);
    http_response_set_reason(res);
    http_response_add_header(res, "Server", "Teapot-v0.0.1");
    http_response_add_header(res, "Connection", "keep-alive");
    http_response_set_body(res,err_body);
    return res;

err_500:
    err_body = "Server Error";
    http_response_set_status(res, ST_INTERNAL_SERVER_ERROR);
    http_response_set_reason(res);
    http_response_add_header(res, "Server", "Teapot-v0.0.1");
    http_response_add_header(res, "Connection", "keep-alive");
    http_response_set_body(res,err_body);
    close(filefd);
    return res;
}

http_response* error_handler(){
    http_response* res = http_response_create();
    if(!res){return NULL;}

    const char *body = "Internal Server Error";

    http_response_set_status(res, ST_INTERNAL_SERVER_ERROR);
    http_response_set_reason(res);
    http_response_add_header(res, "Server", "Teapot-v0.0.1");
    http_response_add_header(res, "Connection", "keep-alive");
    http_response_set_body(res,body);
    return res;
}

void reset_file_stat(file_state* fs){
    free((void*)fs->file_path);
    fs->file_size = 0;
    fs->write_buf->offset = 0;
}
