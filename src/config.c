#include "core/dbuff.h"
#include "core/log.h"
#include "config.h"
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>

static char* find_crlf(dbuffer* src){
    if(!src || src->data == NULL || src->len ==0)
        return NULL;
    char* res = memmem(src->data + src->offset, src->len - src->offset, "\r\n",sizeof("\r\n"));
    return res;
}

static char* find_val(dbuffer* src){
    if(!src || src->data == NULL || src->len ==0)
        return NULL;
    char* res = memmem(src->data + src->offset, src->len - src->offset, "=",sizeof("="));
    res++;
    return res;
}

server_config* config_load(const char* filename){
    dbuffer buff;
    int cfd = open(filename, O_RDONLY);
    if (cfd == -1){
        log_message(LOG_LEVEL_ERROR, "Error in openning config file");
        return NULL;
    }
    io_err res = dbuff_read(&buff, cfd);
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
 
    while(buff.offset < buff.len){
        char* eo_line = find_crlf(&buff);
        if(eo_line == NULL){
            log_message(LOG_LEVEL_ERROR, "Error in finding end of line in config file");
            close(cfd);
            return NULL;
        }

        char* val = find_val(&buff);
        if(val == NULL){
            log_message(LOG_LEVEL_ERROR, "Error in finding end of line in config file");
            close(cfd);
            return NULL;
        }
        while(*val == ' '){
            val++;
        }

        char* key = buff.data + buff.offset;
        char* value = val;

        size_t remain = (buff.data + buff.offset) - eo_line;

        if(memmem(buff.data + buff.offset, remain, "host", strlen("host")) != NULL){
            sc->host = strdup(value);

        }else if(memmem(buff.data + buff.offset, remain, "port", strlen("port")) != NULL){
            int digit = 0;
            int f_port = 0;
            int j = 0;
            while(isdigit(value)){
                digit = value[j] - '0';
                f_port = f_port * 10 + digit;
            }
            sc->port = f_port;

        }else if(memmem(buff.data + buff.offset, remain, "backlog", strlen("backlog")) != NULL){
            int digit = 0;
            int bcl = 0;
            int j = 0;
            while(isdigit(value)){
                digit = value[j] - '0';
                bcl = bcl * 10 + digit;
            }
            sc->backlog = bcl;

        }else if(memmem(buff.data + buff.offset, remain, "document_root", strlen("document_root")) != NULL){
            sc->doc_root = strdup(value);

        }else if(memmem(buff.data + buff.offset, remain, "keep_alive", strlen("keep_alive")) != NULL){
            sc->keep_alive = value[1] - '0' ;

        }else if(memmem(buff.data + buff.offset, remain, "log_level", strlen("log_level")) != NULL){
            sc->log_level = (LLevel)(value[1] - '0');

        }else{
            close(cfd);
            free(sc);
            log_message(LOG_LEVEL_ERROR, "Error in parserign conf file");
            return NULL;
        }
        buff.offset = (buff.data - eo_line) +3;
    }
    return sc;
}

void config_destroy(server_config* sc){
    free(sc);
}
