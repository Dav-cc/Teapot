
#include "server.h"
#include "../core/event.h"
#include "../core/log.h"
#include "sock.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>


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
