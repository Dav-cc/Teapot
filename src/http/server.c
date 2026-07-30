
#include "server.h"
#include "../core/event.h"
#include "../core/log.h"
#include "sock.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>


int init_tcp_server(int port){
    request_handler acceptor = &accept_handler;
    int listenfd = init_listen_socket(port);
    if(listenfd == -1){
        log_message(LOG_LEVEL_ERROR, "error in running server");
        return -1;
    }
    EventLoop* loop = create_EventLoop(1024);
    EventLoop_AddEvent(loop, listenfd, EV_READABLE,NULL, NULL, accept_handler);
    RunEventLoop(loop);
    return 0;
}

int accept_handler(int fd, void* Loop){
    EventLoop* Lp = Loop;
    struct sockaddr_in addr;
    socklen_t socketlen = sizeof(addr);
    for(;;){
        int afd = accept(fd, (struct sockaddr*)&addr, &socketlen);
        if(afd == -1){
            log_message(LOG_LEVEL_ERROR, "error in accept() : %s", strerror(errno));
            return -1;
        }
        sock_set_nonblocking(afd);
        int res = EventLoop_AddEvent(Lp, afd, EV_READABLE, write_handler, read_handler, NULL);
    }
    return 0;
}
