#ifndef __SOCK_H_
#define __SOCK_H_

typedef int(*acceptor)(int fd);

int sock_set_nonblocking(int fd);
int init_listen_socket(int port);
int accept_handler(int fd);

#endif
