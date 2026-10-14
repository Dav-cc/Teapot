#include "dbuff.h"
#include "log.h"
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#define MAX_BUFFER_SIZE (1024 * 64)


dbuffer* dbuff_create(size_t cap){
    if(cap == 0 || cap > MAX_BUFFER_SIZE){
        return NULL;
    }
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
    buffer->offset = 0;

    return buffer;
}

io_err dbuff_read(dbuffer* buf, int fd){
    if(buf == NULL){
        return IO_ERROR;
    }
    int got_data = 0;
    while(1){
        if(buf->cap >= MAX_BUFFER_SIZE){
            return IO_ERROR;
        }
        if(buf->cap - buf->len <= 2048 ){
            ssize_t new_cap = buf->cap * 2;
            char* tmp = realloc(buf->data, new_cap);
            if(!tmp){
                log_message(LOG_LEVEL_ERROR, "error in allocation : %s", strerror(errno));
                // free(buf->data);
                // free(buf);
                return IO_ERROR;
            }
            buf->data = tmp;
            buf->cap = new_cap;
        }
        ssize_t rd_bytes = read(fd, buf->data + buf->len, buf->cap - buf->len);
        if(rd_bytes > 0){    // we read frome socket 
            buf->len += rd_bytes;
            got_data = 1;
            continue;
        }

        if(rd_bytes == -1){  // we sould check errno value
            if(errno == EWOULDBLOCK || errno == EAGAIN){
              return got_data ? IO_DONE : IO_AGAIN;
            }
            if(errno == EINTR){
                continue;
            }
        }
        if(rd_bytes == 0){  // peer closed connection
          return got_data ? IO_DONE : IO_CLOSED;

        }
        return IO_ERROR;
    }
}

io_err dbuff_write(dbuffer* buf, int fd){
    if(buf == NULL){
        return IO_ERROR;
    }
    if (buf->offset > buf->len) {
    return IO_ERROR;
}
    while(buf->len > buf->offset){
        ssize_t wt_bytes = write(fd, buf->data + buf->offset, buf->len - buf->offset);
        if(wt_bytes > 0){
            buf->offset += wt_bytes;
            if(buf->offset > buf->len){
                return IO_ERROR;
            }
            continue;
        }

        if(wt_bytes == -1){
            if(errno == EWOULDBLOCK || errno == EAGAIN){
                return IO_AGAIN;
            }
            if (errno == EINTR) {
              continue;
            }
        }
        return IO_ERROR;
    }
    return IO_DONE;
}

io_err dbuff_append(dbuffer *buf, char *ch, ssize_t len){
    if(len ==0){
        return IO_DONE;
    }
    if(buf == NULL || ch == NULL){
        return IO_ERROR;
    }
    if (len > MAX_BUFFER_SIZE - buf->len) {
        return IO_ERROR;
    }
    while (buf->cap - buf->len < len) {
        if(buf->cap >= MAX_BUFFER_SIZE){
            return IO_ERROR;
        }
        size_t new_cap = buf->cap * 2;

        if(new_cap > MAX_BUFFER_SIZE){
            return IO_ERROR;
        }
        char *tmp = realloc(buf->data, new_cap);
        if (!tmp) {
            log_message(LOG_LEVEL_ERROR,
                        "realloc failed: %s",
                        strerror(errno));
            return IO_ERROR;
        }

        buf->data = tmp;
        buf->cap = new_cap;
    }

    memcpy(buf->data + buf->len, ch, len);
    buf->len += len;

    return IO_DONE;
}

io_err dbuff_reset(dbuffer *buf){
    if(buf == NULL){
        return IO_ERROR;
    }
    buf->offset = 0; 
    buf->len = 0; 
    return IO_DONE;
}

void dbuff_destroy(dbuffer* buf){
    free(buf->data);
    free(buf);
}
