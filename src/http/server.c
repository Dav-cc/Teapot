#define _GNU_SOURCE
#include "server.h"
#include "../core/event.h"
#include "../core/log.h"
#include "sock.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>


Connection* connection_creat(int fd, int is_listener, connection_handler acc, connection_handler readd, connection_handler writee){
    Connection* conn = calloc(1, sizeof(Connection));
    if(!conn){
        log_message(LOG_LEVEL_ERROR, "error in calloc() for conn : %s", strerror(errno));
        return NULL;
    }
    conn->read_buff = db_create(2048);
    if(!conn->read_buff){
        log_message(LOG_LEVEL_ERROR,"error in creatint read buffer, %s ", strerror(errno));
        free(conn);
    }
    conn->write_buff = db_create(2048);
    if(!conn->write_buff){
        log_message(LOG_LEVEL_ERROR,"error in creatint write buffer, %s ", strerror(errno));
        free(conn->read_buff);
        free(conn);
    }
    memset(&conn->req, 0, sizeof(conn->req)); // parser init

    conn->fd = fd;
    conn->rlen = 0;
    conn->wlen = 0;
    conn->keep_alive = 1;
    conn->accept_func= acc;
    conn->read_func = readd;
    conn->write_func = writee;
    if(is_listener) conn->listener = 1;
    return conn;
}
int connection_destroy(Connection* conn){
    log_message(LOG_LEVEL_INFO,"Closing connecting- fd = %d, ", conn->fd);
    rb_destroy(conn->read_buff);
    rb_destroy(conn->write_buff);
    close(conn->fd);
    free(conn);
    return 0;
}

int init_tcp_server(int port){
    connection_handler acceptor = &accept_handler;
    int listenfd = init_listen_socket(port);
    if(listenfd == -1){
        log_message(LOG_LEVEL_ERROR, "error in running server");
        return -1;
    }
    Connection* listen_conn = connection_creat(listenfd, 1, acceptor, NULL,NULL);
    EventLoop* loop = create_EventLoop(1024);
    EventLoop_AddEvent(loop, listen_conn, EV_READABLE);
    RunEventLoop(loop);
    return 0;
}

int accept_handler(Connection* conn, void* Loop){
    EventLoop* Lp = Loop;
    connection_handler read_handle = &read_handler;
    connection_handler write_handle = &write_handler;
    struct sockaddr_in addr;
    for(;;){
        socklen_t socketlen = sizeof(addr);
        int afd = accept4(conn->fd, (struct sockaddr*)&addr, &socketlen, SOCK_NONBLOCK|SOCK_CLOEXEC);
        if(afd == -1){
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            log_message(LOG_LEVEL_ERROR, "error in accept() : %s", strerror(errno));
            return -1;
        }
        sock_set_keep_alive(afd);
        Connection* accept_conn = connection_creat(afd, 0, NULL, read_handle, write_handle);
        int res = EventLoop_AddEvent(Lp, accept_conn , EV_READABLE);
    }
    return 0;
}
