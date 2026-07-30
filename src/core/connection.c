#include "connection.h"

Connection* connection_creat(int fd, int is_listener){
    Connection* conn = calloc(1, sizeof(Connection));
    conn->fd = fd;
    conn->rlen = 0;
    conn->wlen = 0;
    if(is_listener) conn->listener =1;
    return conn;
}
int connection_destroy(int fd, Connection* conn){
    close(fd);
    free(conn);
    return 0;
}
