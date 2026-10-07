#ifndef STATIC_TEAPOT_H
#define STATIC_TEAPOT_H

#include "../http/http_response.h"
#include "../http/router.h"

typedef struct {
    const char* type;
    const char* mime;
}mime_type_table;

http_response* static_response_init(http_request* req);
char* path_verifyer(http_request* req);
file_state* get_file_state(int filefd);
file_state* find_mime_type(file_state* fs, char* path);
#endif
