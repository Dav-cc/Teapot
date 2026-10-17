#ifndef __SOCK_H_
#define __SOCK_H_

#include "server.h"
#include "../core/dbuff.h"
#include "../config.h"

// Global server setting
extern server_config* G_config;


int sock_set_nonblocking(int fd);
int sock_set_keep_alive(int fd);
int init_listen_socket(server_config *sc);
// int accept_handler(Connection* conn, void* Loop);
int write_handler(EventLoop* loop, FileEvent* fe);
int read_handler(EventLoop* loop, FileEvent* fe);

#endif
