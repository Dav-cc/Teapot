#include "core/dbuff.h"
#include "core/log.h"
#include "config.h"
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>

server_config *G_config = NULL;

static char *slice_dup(const char *start, size_t len)
{
    char *s = malloc(len + 1);
    if (!s)
        return NULL;

    memcpy(s, start, len);
    s[len] = '\0';

    return s;
}

static char* find_crlf(dbuffer* src){
    if(!src || src->data == NULL || src->offset > src->len)
        return NULL;
    char* res = memchr(src->data + src->offset, '\n',src->len - src->offset);
    return res;
}

static char* find_val(dbuffer* src){
    if(!src || src->data == NULL || src->len ==0)
        return NULL;
    char* res = memchr(src->data + src->offset, '=',src->len - src->offset);
    res++;
    return res;
}

server_config* config_load(const char* filename){
    int cfd = open(filename, O_RDONLY);
    if (cfd == -1){
        log_message(LOG_LEVEL_ERROR, "Error in openning config file");
        return NULL;
    }
    dbuffer* buff = dbuff_create(1024);
    io_err res = dbuff_read(buff, cfd);
    if(res == IO_ERROR || res == IO_AGAIN){
        log_message(LOG_LEVEL_ERROR, "Error in reading config file content");
        close(cfd);
        return NULL;
    }
    server_config* sc = calloc(1, sizeof(server_config));
    if(!sc){
        log_message(LOG_LEVEL_ERROR, "Error in initializing memory for server_config");
        close(cfd);
        return NULL;
    }
 
    while(buff->offset < buff->len){
        char* eo_line = find_crlf(buff);
        if(eo_line == NULL){
            log_message(LOG_LEVEL_ERROR, "Error in finding end of line in config file");
            close(cfd);
            return NULL;
        }

        char* val = find_val(buff);
        if(val == NULL){
            log_message(LOG_LEVEL_ERROR, "Error in finding end of line in config file");
            close(cfd);
            return NULL;
        }
        while(*val == ' '){
            val++;
        }

        char* value = val;

        size_t remain = (size_t)(eo_line - (buff->data + buff->offset));

        if(memmem(buff->data + buff->offset, remain, "host", strlen("host")) != NULL){
            sc->host = slice_dup(value, eo_line - val);

        }else if(memmem(buff->data + buff->offset, remain, "port", strlen("port")) != NULL){
            int digit = 0;
            int f_port = 0;
            int j = 0;
            while(isdigit(value[j])){
                digit = value[j] - '0';
                f_port = f_port * 10 + digit;
                j++;
            }
            sc->port = f_port;

        }else if(memmem(buff->data + buff->offset, remain, "backlog", strlen("backlog")) != NULL){
            int digit = 0;
            int bcl = 0;
            int j = 0;
            while(isdigit(value[j])){
                digit = value[j] - '0';
                bcl = bcl * 10 + digit;
                j++;
            }
            sc->backlog = bcl;

        }else if(memmem(buff->data + buff->offset, remain, "document_root", strlen("document_root")) != NULL){
            sc->doc_root = slice_dup(value, eo_line - value);

        }else if(memmem(buff->data + buff->offset, remain, "log_level", strlen("log_level")) != NULL){
            sc->log_level = (int)(value[0] - '0');

        }else{
            close(cfd);
            free(sc);
            log_message(LOG_LEVEL_ERROR, "Error in parserign conf file");
            return NULL;
        }
        buff->offset = (size_t)(eo_line - buff->data) +1;
    }
    return sc;
}

void config_destroy(server_config* sc){
    if(!sc)
        return ;
    free(sc);
}
