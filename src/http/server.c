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
#include "../parser/http_parser.h"


Connection* connection_creat(int fd,int is_listener, event_callback readd, event_callback writee){
    Connection* conn = calloc(1, sizeof(Connection));
    if(!conn){
        log_message(LOG_LEVEL_ERROR, "error in calloc() for conn : %s", strerror(errno));
        return NULL;
    }
    conn->read_buff = dbuff_create(2048);
    if(!conn->read_buff){
        log_message(LOG_LEVEL_ERROR,"error in creating read buffer, %s ", strerror(errno));
        free(conn);
        return NULL;
    }
    conn->write_buff = dbuff_create(2048);
    if(!conn->write_buff){
        log_message(LOG_LEVEL_ERROR,"error in creating write buffer, %s ", strerror(errno));
        free(conn->read_buff);
        free(conn);
        return NULL;
    }
    //TODO: Parser instance
    conn->fd = fd;
    conn->filev.mask = EV_READABLE | EV_ET;
    conn->filev.fd = conn->fd;
    conn->keep_alive = 1;
    conn->filev.callbacks.on_read = readd;
    conn->filev.callbacks.on_write = writee;
    conn->filev.data = conn;

    if(is_listener) conn->listener = 1;
    return conn;
}
int connection_destroy(Connection* conn){
    log_message(LOG_LEVEL_INFO,"Closing connecting- fd = %d, ", conn->fd);
    dbuff_destroy(conn->read_buff);
    dbuff_destroy(conn->write_buff);
    close(conn->fd);
    free(conn);
    return 0;
}

int init_tcp_server(int port){
    int listenfd= init_listen_socket(port);
    if(listenfd == -1){
        log_message(LOG_LEVEL_ERROR, "error in running server");
        return -1;
    }
    Connection* listen_conn = connection_creat(listenfd, 1, NULL,NULL);
    EventLoop* loop = eventloop_create(1024);

    listen_conn->filev.callbacks.on_read = accept_handler;
    listen_conn->filev.mask = EV_READABLE | EV_ET;
    listen_conn->filev.fd = listen_conn->fd;
    
    eventloop_add_event(loop, &listen_conn->filev);
    eventloop_run(loop);
    return 0;
}

int accept_handler(EventLoop *loop, FileEvent *fe) {
  struct sockaddr_in addr;
  for (;;) {
    socklen_t socketlen = sizeof(addr);
    int afd = accept4(fe->fd, (struct sockaddr *)&addr, &socketlen,
                      SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (afd == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        break;
      log_message(LOG_LEVEL_ERROR, "error in accept() : %s", strerror(errno));
      return -1;
    }
    sock_set_keep_alive(afd);
    Connection *accept_conn = connection_creat(afd, 0, read_handler, write_handler);
    if(!accept_conn){
        close(afd);
        continue;
    }
    accept_conn->filev.mask = EV_READABLE | EV_ET;
    accept_conn->filev.data = accept_conn;
    eventloop_add_event(loop, &accept_conn->filev);
  }
  return 0;
}
