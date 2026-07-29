
#include "server.h"
#include "../core/event.h"
#include "../core/log.h"
#include "sock.h"
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>

typedef int(*acceptor)(int fd);

int accept_handler(int fd){
    struct sockaddr_in addr;
    socklen_t socketlen = sizeof(addr);
    int afd = accept(fd, (struct sockaddr*)&addr, &socketlen);
    if(afd == -1){
        log_message(LOG_LEVEL_ERROR, "error in accepting : %s", strerror(errno));
    }
    log_message(LOG_LEVEL_INFO," got connection from fd = %d", afd);
    close(afd);
    return 0;
}

int init_tcp_server(int port){
    acceptor acc = &accept_handler;
    int listenfd = init_listen_socket(port);
    if(listenfd == -1){
        log_message(LOG_LEVEL_ERROR, "error in running server");
        return -1;
    }
    EventLoop* loop = create_EventLoop(1024);
    EventLoop_AddEvent(loop, listenfd, EV_READABLE,accept_handler);
    RunEventLoop(loop);
    return 0;
}
