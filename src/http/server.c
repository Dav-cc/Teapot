// #include "../core/event.h"
// #include "../core/log.h"
// #include "../http/sock.h"
// #include <sys/socket.h>
// #include <unistd.h>
//
// int init_tcp_server(int port){
//     int res = create_EventLoop(1024, AcceptHandler handler, ReadHandler read, WriteHandler write);
//     int listenfd = init_listen_socket(port);
//
// }
#include "../core/event.h"
#include "../core/log.h"
#include "../http/sock.h"

#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>

static EventLoop *g_el = NULL;
static int g_listenfd = -1;

void accept_handler(int fd, int flags)
{
    (void)flags;

    if(fd != g_listenfd)
        return;

    while(1)
    {
        int clientfd = accept(fd, NULL, NULL);

        if(clientfd == -1)
        {
            if(errno == EAGAIN || errno == EWOULDBLOCK)
                break;

            log_message(
                LOG_LEVEL_ERROR,
                "accept failed: %s",
                strerror(errno)
            );
            return;
        }

        log_message(
            LOG_LEVEL_INFO,
            "new client fd=%d",
            clientfd
        );

        close(clientfd);
    }

}

void read_handler(int fd, int flags)
{
    (void)fd;
    (void)flags;
}

void write_handler(int fd, int flags)
{
    (void)fd;
    (void)flags;
}

int init_tcp_server(int port)
{
    g_el = create_EventLoop(
        1024,
        accept_handler,
        read_handler,
        write_handler
    );

    if(!g_el)
        return -1;

    g_listenfd = init_listen_socket(port);
    if(g_listenfd == -1)
        return -1;

    if(EventLoop_AddFd(
            g_el,
            g_listenfd,
            FD_REDABLE) == -1)
    {
        return -1;
    }

    log_message(
        LOG_LEVEL_INFO,
        "server listening on port %d",
        port
    );

    RunEventLoop(g_el);

    return 0;
}
