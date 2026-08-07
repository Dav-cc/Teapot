#ifndef RB_TEAPOT_H
#define RB_TEAPOT_H

#include <stdlib.h>
#include <unistd.h>

typedef struct buffer{
    char* data;
    size_t len;
    size_t cap;

    size_t offset;
}buffer_t;

buffer_t *db_create(size_t size);
void rb_destroy(buffer_t *rb);
ssize_t db_socket_read(buffer_t *db, int fd);
ssize_t db_socket_write(buffer_t *db, int fd);
#endif
