#ifndef RB_TEAPOT_H
#define RB_TEAPOT_H

#include <stdlib.h>

typedef struct RingBuffer {
    char *data;
    size_t size;
    size_t head;
    size_t tail;
} RingBuffer;

RingBuffer *rb_create(size_t size);
void rb_destroy(RingBuffer *rb);
size_t rb_readable(RingBuffer *rb);
size_t rb_writable(RingBuffer *rb);
size_t rb_write(RingBuffer *rb,void *src, size_t len);
size_t rb_read(RingBuffer *rb,void *dst, size_t len);
void rb_reset(RingBuffer *rb);

#endif
