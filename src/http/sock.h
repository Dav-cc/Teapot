#ifndef __SOCK_H_
#define __SOCK_H_

#include "server.h"



int sock_set_nonblocking(int fd);
int sock_set_keep_alive(int fd);
int init_listen_socket(int port);
int accept_handler(Connection* conn, void* Loop);
int write_handler(Connection* conn, void* Loop);
int read_handler(Connection* conn, void* Loop);

#endif
