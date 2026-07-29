#ifndef SERVER_H
#define SERVER_H

typedef int(*request_handler)(int fd, void* Loop);

int accept_handler(int fd, void* Loop);
int init_tcp_server(int port);


#endif
