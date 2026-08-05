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

void rb_destroy(RingBuffer *rb){
    if (!rb)
        return;
    free(rb->data);
    free(rb);
}
