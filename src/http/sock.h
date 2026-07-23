#ifndef __SOCK_H_
#define __SOCK_H_


int setsock_nonblocking(int sockfd, int nonblocking);
int init_listen_socket(int port);
#endif
