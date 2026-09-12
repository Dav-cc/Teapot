#ifndef SERVER_H
#define SERVER_H

#include "../core/event.h"
#include "../parser/http_parser.h"
#include "../core/dbuff.h"

typedef struct Connection Connection;

int accept_handler(EventLoop* loop, FileEvent* fe);
int init_tcp_server(int port);
Connection* connection_creat(int fd, int is_listener, event_callback readd, event_callback writee);
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

    dbuffer* read_buff;
    dbuffer* write_buff;

    FileEvent filev;
    // connection_handler accept_func;
    // connection_handler write_func;
    // connection_handler read_func;


    http_parser_state_t pstate;
    http_parser_t* parser;
};

#endif
