#ifndef __TEAPOT_CONN_H_
#define __TEAPOT_CONN_H_

#include <stdlib.h>
#include <unistd.h>

typedef struct Connection{
    int fd;
    int listener;
    int rlen;
    int wlen;
    char rbuf[1024];
    char wbuf[1024];
}Connection;

Connection* connection_creat(int fd, int is_listener);
int connection_destroy(int fd, Connection* conn);

#endif
