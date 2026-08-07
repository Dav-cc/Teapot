#ifndef SERVER_H
#define SERVER_H

#include "../parser/parser.h"
#include "../core/rb.h"
typedef struct Connection Connection;
typedef int(*connection_handler)(Connection* conn, void* Loop);

int accept_handler(Connection* conn, void* Loop);
int init_tcp_server(int port);
Connection* connection_creat(int fd, int is_listener, connection_handler acc, connection_handler readd, connection_handler writee);
int connection_destroy(Connection* conn);

typedef enum {
    CONN_READING,
    CONN_WRITING,
}conn_state;

struct Connection{
    int fd;
    int listener;

    int rlen;
    int wlen;

    int keep_alive;

    conn_state state; 

    buffer_t* read_buff;
    buffer_t* write_buff;

    connection_handler accept_func;
    connection_handler write_func;
    connection_handler read_func;

    http_request_t req;
    http_parser_t parser;
};

#endif
