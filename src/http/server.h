#ifndef SERVER_H
#define SERVER_H

typedef struct Connection Connection;
typedef int(*connection_handler)(Connection* conn, void* Loop);

int accept_handler(Connection* conn, void* Loop);
int init_tcp_server(int port);
Connection* connection_creat(int fd, int is_listener, connection_handler acc, connection_handler readd, connection_handler writee);
int connection_destroy(Connection* conn);

typedef enum {
    CONN_RECIEVING = 0,
    CONN_READING,
    CONN_WRITING,
    CONN_ERR,
}conn_state;

struct Connection{
    int fd;
    int listener;
    int rlen;
    int wlen;
    conn_state state; 
    char* rbuff;
    char* wbuff;
    connection_handler accept_func;
    connection_handler write_func;
    connection_handler read_func;
};

#endif
