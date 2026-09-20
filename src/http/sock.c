#include "sock.h"
#include "http_response.h"
#include "router.h"
#include "server.h"
#include "../core/log.h"
#include "../core/event.h"
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>


int idle = 60;
int count = 5;
int interval = 15;
 
int sock_set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        log_message(LOG_LEVEL_ERROR, "error in fnctl() :%s", strerror(errno));
        return -1;
    }
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int sock_set_keep_alive(int fd) {
    int yes = 1;

    int idle = 60;
    int interval = 10;
    int count = 3;

    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(yes)) == -1) {
      log_message(LOG_LEVEL_INFO, "setsockopt keep-alive: %s", strerror(errno));
      return -1;
    }

    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(idle)) == -1) {
      log_message(LOG_LEVEL_INFO, "setsockopt keep-alive: %s", strerror(errno));
      return -1;
    }
    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &interval,
                   sizeof(interval)) == -1) {
      log_message(LOG_LEVEL_INFO, "setsockopt keep-alive: %s", strerror(errno));
      return -1;
    }
    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &count, sizeof(count)) == -1) {
      log_message(LOG_LEVEL_INFO, "setsockopt keep-alive: %s", strerror(errno));
      return -1;
    }

    return 0;
}

int sock_set_reuseaddr(int fd) {
    int yes = 1;
    if (setsockopt(fd, SOL_SOCKET,SO_REUSEADDR, &yes,sizeof(yes)) == -1) {
        log_message(LOG_LEVEL_ERROR, "error in sockopt() :%s", strerror(errno));
        return -1;
    }
    return 1;
}
int sock_set_nodelay(int fd) {
    int yes = 1;
    if (setsockopt(fd, IPPROTO_TCP,TCP_NODELAY, &yes,sizeof(yes)) == -1) {
        log_message(LOG_LEVEL_ERROR, "error in sockopt() :%s", strerror(errno));
        return -1;
    }
    return 1;
}

int write_handler(EventLoop *loop, FileEvent *fe) {
    Connection* conn = fe->data;
    conn->state = CONN_WRITING;

    io_err err_code = dbuff_write(conn->write_buff, conn->fd);
    log_message(LOG_LEVEL_DEBUG, "response sent fd=%d keep_alive=%d", conn->fd,
                conn->request->keep_alive);

    switch (err_code){
        case IO_DONE:{
            if(conn->request->keep_alive == 0){
                eventloop_del_event(loop, &conn->filev);
                connection_destroy(conn);
                return 0;
            }
            conn->state = CONN_READING;

            dbuff_reset(conn->read_buff);
            dbuff_reset(conn->write_buff);

            eventloop_mod_event(loop, &conn->filev, EV_READABLE | EV_ET);
            return 0;
        }
        case IO_AGAIN:
            return 0; 

        case IO_CLOSED:
        case IO_ERROR:
            eventloop_del_event(loop, &conn->filev);
            connection_destroy(conn);
            return 0;
    }
    return 0;
}

int read_handler(EventLoop* loop, FileEvent* fe){
    size_t consumed = 0;
    Connection* conn = fe->data;
    conn->state = CONN_READING;

    io_err err_code = dbuff_read(conn->read_buff,conn->fd);

    if (err_code == IO_AGAIN)
      return 0;

    if (err_code == IO_CLOSED || err_code == IO_ERROR) {
      eventloop_del_event(loop, &conn->filev);
      connection_destroy(conn);
      return 0;
    }

    conn->request->result = http_parser_parse(conn, conn->read_buff);

    switch (conn->request->result) {
        case PARSER_ERROR:
            log_message(LOG_LEVEL_INFO, "parsing error on fd = %d",conn->fd);
            eventloop_del_event(loop, &conn->filev);
            connection_destroy(conn);
            return 0;

        case PARSER_COMPLETE:
            break;

        case PARSER_NEED_MORE:
            // still in read 
            return 0;
    }
    router* r = find_router(conn->request);
    if(r == NULL){
        eventloop_del_event(loop, &conn->filev);
        connection_destroy(conn);
        return 0;
    }
    http_response *res = r->rout_handler(conn->request);

    size_t w = http_response_serializer(res, conn->write_buff);
    if (w == 0) {
      log_message(LOG_LEVEL_ERROR, "failed to serianlize on fd = %d", conn->fd);
      eventloop_del_event(loop, &conn->filev);
      connection_destroy(conn);
      return 0;
    }

    conn->state = CONN_WRITING;

    eventloop_mod_event(loop, &conn->filev, EV_WRITABLE | EV_ET);

    return 0;
}

int init_listen_socket(int port) {
    int yes =1;
    struct sockaddr_in addr = {0};

    // creat tcp ipv4 socket
    int sockfd = socket(AF_INET, SOCK_STREAM|SOCK_NONBLOCK|SOCK_CLOEXEC, 0);
    if(sockfd < 0){
        log_message(LOG_LEVEL_ERROR, "socket() failed : %s", strerror(errno));
        return -1;
    }
    sock_set_reuseaddr(sockfd);
    sock_set_nodelay(sockfd);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    int res = bind(sockfd, (struct sockaddr*)&addr, sizeof(addr));
    if(res < 0){
        log_message(LOG_LEVEL_ERROR, "bind() failed : %s", strerror(errno));
        close(sockfd);
        log_message(LOG_LEVEL_INFO,"CLOSE fd=%d",sockfd);
        return -1;
    }

    int listen_res = listen(sockfd, SOMAXCONN);
    if(listen_res < 0){
        log_message(LOG_LEVEL_ERROR, "listen() failed : %s", strerror(errno));
        close(sockfd);
        log_message(LOG_LEVEL_INFO,"CLOSE fd=%d",sockfd);
        return -1;
    }
    sock_set_nonblocking(sockfd);
    log_message(LOG_LEVEL_INFO, "Listening on port %d", port);
    return sockfd;
}



