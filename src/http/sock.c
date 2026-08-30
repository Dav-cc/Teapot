#include "sock.h"
#include "server.h"
#include "../core/log.h"
#include "../core/event.h"
#include "../http/http_response.h"
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

int write_handler(Connection *conn, void *Loop) {
    EventLoop *Lp = Loop;
    conn->state = CONN_WRITING;

    ssize_t writed = db_socket_write(conn->write_buff, conn->fd);

    if (writed == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          log_message(LOG_LEVEL_DEBUG, "fd=%d recieved EAGAIN signal", conn->fd);
          return 0;
        }
        log_message(LOG_LEVEL_ERROR, "write error fd=%d : %s", conn->fd,strerror(errno));
        eventloop_del_event(Lp, conn);
        connection_destroy(conn);
        return -1;
    }

    if (conn->write_buff->offset < conn->write_buff->len) {
      eventloop_mod_event(Lp, conn, EV_WRITABLE);
      return 0;
    }

    /* complete response sent */
    conn->write_buff->offset = 0;
    conn->write_buff->len = 0;

    eventloop_mod_event(Lp, conn, EV_READABLE);

    // log_message(LOG_LEVEL_INFO, "fd=%d writed %ld bytes", conn->fd, writed);
    // EventLoop_ModEvent(Lp, conn, EV_READABLE);
    // return 0;
}

int read_handler(Connection* conn, void* Loop){
    size_t consumed = 0;
    EventLoop* Lp = Loop;
    conn->state = CONN_READING;
    ssize_t readed = db_socket_read(conn->read_buff,conn->fd);
    conn->rlen = readed;
    if(readed == 0){
        log_message(LOG_LEVEL_INFO, "client closing connection, fd = %d closed", conn->fd);
        eventloop_del_event(Lp, conn);
        connection_destroy(conn);
        return -1;
    }
    if(readed == -1){
        log_message(LOG_LEVEL_ERROR, "peer fd = %d errored sent no data", conn->fd);
        connection_destroy(conn);
        return -1;
    }
     log_message(LOG_LEVEL_INFO, "fd = %d\n recived  buffer in %d bytes", conn->fd, readed);

    log_message(LOG_LEVEL_INFO, "buffer going for parse");
    
    // TODO: implement this correct
    conn->rlen = conn->read_buff->len;
    http_parser_result_t res = http_parser_parse(conn->parser, conn->read_buff, conn->rlen, &consumed);

    switch (res) {
        case PARSER_RESULT_OK:
            log_message(LOG_LEVEL_INFO, "parsing complete fd = %d",conn->fd);
            http_response_builder(conn->parser,&conn->parser->request, conn, Loop);
            // db_socket_write(conn->write_buff,conn->fd);
            return 0;
        
        case PARSER_RESULT_NEED_MORE:
            return 0;
    }

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



