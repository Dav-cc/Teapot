#ifndef __DBUFF_H
#define __DBUFF_H

#include <stdlib.h>

typedef struct {
    char* data;         // this pointer always points to starting point of buffer
    size_t cap;        // capacity of buffer which is going to be bigger if it gets full
    size_t len;        // how much it is full at the moment
    size_t offset;     // where we are at current momment in buffer (for parsing , read/write  etc . . . )
} dbuffer;

typedef enum{
    IO_DONE = 0,
    IO_ERROR = 1,
    IO_AGAIN = 2,
    IO_CLOSED = 3,
}io_err;

dbuffer* dbuff_create(size_t cap);
void dbuff_destroy(dbuffer* buf);
io_err dbuff_read(dbuffer* buf, int fd);
io_err dbuff_write(dbuffer* buf ,int fd);
io_err dbuff_append(dbuffer* buf, char* ch);

#endif
