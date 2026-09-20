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
    buffer->offset = 0;

    return buffer;
}

io_err dbuff_read(dbuffer* buf, int fd){
    int got_data = 0;
    while(1){
        if(buf->cap - buf->len <= 2048){
            ssize_t new_cap = buf->cap * 2;
            char* tmp = realloc(buf->data, new_cap);
            if(!tmp){
                log_message(LOG_LEVEL_ERROR, "error in allocation : %s", strerror(errno));
                free(buf->data);
                free(buf);
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
              log_message(LOG_LEVEL_INFO, "READ fd=%d -> EAGAIN got_data=%d",
                          fd, got_data);
              return got_data ? IO_DONE : IO_AGAIN;
            }
            if(errno == EINTR){
                continue;
            }
        }
        if(rd_bytes == 0){  // peer closed connection
          log_message(LOG_LEVEL_INFO, "READ fd=%d -> EOF",fd); 
          log_message(LOG_LEVEL_DEBUG, "closing fd=%d reason=???", fd);
          log_message(LOG_LEVEL_INFO, "peer closed connection");
          return IO_CLOSED;
        }
        return IO_ERROR;
    }
}

io_err dbuff_write(dbuffer* buf, int fd){
    while(buf->len > buf->offset){
        int wt_bytes = write(fd, buf->data + buf->offset, buf->len - buf->offset);
        if(wt_bytes > 0){
            buf->offset += wt_bytes;
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

io_err dbuff_append(dbuffer *buf, char *ch, size_t len)
{
    while (buf->cap - buf->len < len) {
        size_t new_cap = buf->cap * 2;

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
    buf->offset = 0; 
    buf->len = 0; 
    return IO_DONE;
}

void dbuff_destroy(dbuffer* buf){
    free(buf->data);
    free(buf);
}
