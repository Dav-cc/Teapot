#include "sock.h"
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

int sock_set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        log_message(LOG_LEVEL_ERROR, "error in fnctl() :%s", strerror(errno));
        return -1;
    }
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
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

// int accept_handler(int fd, void* Loop){
//     EventLoop* Lp = Loop;
//     struct sockaddr_in addr;
//     socklen_t socketlen = sizeof(addr);
//     for(;;){
//         int afd = accept(fd, (struct sockaddr*)&addr, &socketlen);
//         if(afd == -1){
//             log_message(LOG_LEVEL_ERROR, "error in accept() : %s", strerror(errno));
//             return -1;
//         }
//         sock_set_nonblocking(afd);
//         int res = EventLoop_AddEvent(Lp, afd, EV_READABLE, write_handler, read_handler, NULL);
//     }
//     return 0;
// }


int write_handler(int fd, void* Loop){
    EventLoop* Lp = Loop;
    char buffer[70] = "HTTP/1.1 200 OK\r\nContent-Length: 6\r\n\r\nhello\n";
    write(fd, buffer, sizeof(buffer));
    EventLoop_DelEvent(Lp, fd);
    close(fd);
    log_message(LOG_LEVEL_INFO,"CLOSE fd=%d",fd);
    log_message(LOG_LEVEL_INFO, "sended buffer = [[%s]]\n connection closed", buffer);
    return 0;
}

int read_handler(int fd, void* Loop){
    EventLoop* Lp = Loop;
    char buffer[1024];
    read(fd, buffer, sizeof(buffer));
    log_message(LOG_LEVEL_INFO, " --- Recive Buffer ---\n %s", buffer);

    log_message(LOG_LEVEL_INFO, "changing fd mod to writeable");
    EventLoop_ModEvent(Lp,fd, EV_WRITABLE);
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



