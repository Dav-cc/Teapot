#ifndef TEAPOT_CONF_H
#define TEAPOT_CONF_H

#include "core/log.h"

typedef struct {
    char* host;
    unsigned short port;
    int backlog; 

    char* doc_root;

    LLevel log_level;
} server_config;


server_config* config_load(const char* filename);
void config_destroy(server_config* sc);


#endif
