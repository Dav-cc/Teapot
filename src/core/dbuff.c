#include "dbuff.h"
#include "log.h"
#include <asm-generic/errno.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>


dbuffer* dbuff_create(size_t cap){
    dbuffer* buffer = calloc(1, sizeof(dbuffer));
    if(!buffer){
        log_message(LOG_LEVEL_ERROR, "error in allocation : %s", strerror(errno));
        return NULL;
    }
    buffer->data = calloc(1, cap);
    if(!buffer->data){
        log_message(LOG_LEVEL_ERROR, "error in allocation: %s", strerror(errno));
        free(buffer);
        return NULL;
    }
    buffer->cap = cap;
    buffer->len = 0;
    buffer->data = NULL;
    buffer->offset = 0;

    return buffer;
}

size_t dbuff_read(dbuffer* buf, int fd){
    while(1){
        if(buf->cap - buf->len <= 2048){
            ssize_t new_cap = buf->cap * 2;
            char* tmp = realloc(buf->data, new_cap);
            if(!tmp){
                log_message(LOG_LEVEL_ERROR, "error in allocation : %s", strerror(errno));
                free(buf->data);
                free(buf);
                return -1;
            }
            buf->data = tmp;
            buf->cap = new_cap;
        }
        ssize_t rd_bytes = read(fd, buf->data + buf->len, buf->cap - buf->len);
        buf->len += rd_bytes;
        if(rd_bytes > 0){    // we read frome socket 
            continue;
        }

        if(rd_bytes == -1){  // we sould check errno value
            if(errno == EWOULDBLOCK || errno == EAGAIN){
                break;
            }
            if(errno == EINTR){
                continue;
            }
        }
        if(rd_bytes == 0){  // peer closed connection 
            // TODO: handle closing connection here
            log_message(LOG_LEVEL_INFO," peer closed connection");
            return -1;
        }
    }
    return buf->len;
}

size_t dbuff_write(dbuffer* buf, int fd){
    while(buf->len > buf->offset){
        int wt_bytes = write(fd, buf->data + buf->offset, buf->len - buf->offset);
        buf->offset += wt_bytes;

        if(wt_bytes > 0){
            continue;
        }

        if(wt_bytes == -1){
            if(errno == EWOULDBLOCK || errno == EAGAIN)
                break;
        }
        return -1;
    }
    return buf->offset;
}


void dbuff_destroy(dbuffer* buf){
    free(buf->data);
    free(buf);
}
