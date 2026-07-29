#ifndef SERVER_H
#define SERVER_H

typedef int(*request_handler)(int fd);

int accept_handler(int fd);
int init_tcp_server(int port);


#endif
