#include "rb.h"
#include "log.h"
#include <string.h>
#include <errno.h>



RingBuffer *rb_create(size_t size){
    RingBuffer* rb = calloc(1, sizeof(RingBuffer));
    if(!rb){
        log_message(LOG_LEVEL_ERROR, "error in alocation for rb: %s", strerror(errno));
        return NULL;
    }
    rb->data = calloc(size, sizeof(char));
    if(!rb->data){
        free(rb);
        log_message(LOG_LEVEL_ERROR, "error in alocation rb->data: %s", strerror(errno));
        return NULL;
    }
    rb->head = 0;
    rb->size = size;
    rb->tail = 0;
    return rb;
}

size_t rb_readable(RingBuffer *rb){
    return (((rb->size + rb->tail - rb->head ))%rb->size);
}
size_t rb_writable(RingBuffer *rb){
    return ((rb->size - rb_readable(rb)) - 1);
}

size_t rb_write(RingBuffer *rb, void *src, size_t len){
    const char* data = src;
    size_t writable = rb_writable(rb);
    if(len > writable){
        len = writable;
    }
    for(size_t i = 0; i < len; i++){
        rb->data[rb->tail] = data[i];
        rb->tail = (rb->tail + 1) % rb->size;
    }
    return len;
}

size_t rb_read(RingBuffer *rb, void *dst, size_t len) {
    char *out = dst;

    size_t readable = rb_readable(rb);

    if (len > readable)
        len = readable;

    for (size_t i = 0; i < len; i++) {

    out[i] = rb->data[rb->head];

    rb->head = (rb->head + 1) % rb->size;
    }

    return len;
}

size_t rb_contiguous_writable(RingBuffer *rb){
    if(rb->tail >= rb->head)
        return rb->size - rb->tail;

    return rb->head - rb->tail - 1;
}

ssize_t rb_socket_read(RingBuffer *rb, int fd) {
    size_t space = rb_contiguous_writable(rb);

    log_message(LOG_LEVEL_DEBUG, "rb_read fd=%d head=%zu tail=%zu writable=%zu",fd, rb->head, rb->tail, space);
    if (space == 0) {
    log_message(LOG_LEVEL_WARN, "rb_read fd=%d buffer full", fd);
    errno = ENOBUFS;
    return -1;
    }
    ssize_t n = read(fd, rb->data + rb->tail, space);
    if (n > 0) {
    rb->tail = (rb->tail + n) % rb->size;
    log_message(LOG_LEVEL_DEBUG, "rb_read fd=%d read=%zd new_tail=%zu", fd, n, rb->tail);
    } else if (n == 0) {
    log_message(LOG_LEVEL_INFO, "rb_read fd=%d peer closed connection", fd);
    } else {
    log_message(LOG_LEVEL_DEBUG, "rb_read fd=%d error=%s", fd, strerror(errno));
    }
    return n;
}

ssize_t rb_socket_write(RingBuffer *rb, int fd) {
    size_t available;

    if (rb->head < rb->tail)
        available = rb->tail - rb->head;
    else
        available = rb->size - rb->head;

    log_message(LOG_LEVEL_DEBUG, "rb_write fd=%d head=%zu tail=%zu readable=%zu",fd, rb->head, rb->tail, available);
    if (available == 0) {
    log_message(LOG_LEVEL_DEBUG, "rb_write fd=%d buffer empty", fd);
    return 0;
    }
    ssize_t n = write(fd, rb->data + rb->head, available);
    if (n > 0) {
    rb->head = (rb->head + n) % rb->size;
    log_message(LOG_LEVEL_DEBUG, "rb_write fd=%d sent=%zd new_head=%zu", fd, n,rb->head);
    } else {
    log_message(LOG_LEVEL_DEBUG, "rb_write fd=%d error=%s", fd,strerror(errno));
    }
    return n;
}

void rb_destroy(RingBuffer *rb){
    if (!rb)
        return;
    free(rb->data);
    free(rb);
}
