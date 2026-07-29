#ifndef __SOCK_H_
#define __SOCK_H_

int sock_set_nonblocking(int fd);
int init_listen_socket(int port);
int accept_handler(int fd, void* Loop);
int write_handler(int fd, void* Loop);
int read_handler(int fd, void* Loop);

#endif
