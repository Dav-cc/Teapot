#include "log.h"
#include "event.h"
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

EventLoop *event_loop_create(int max_events) {
    EventLoop *loop = (EventLoop *)malloc(sizeof(EventLoop));
    if (!loop) {
        log_message(LOG_LEVEL_ERROR, "Failed to allocate memory for EventLoop: %s", strerror(errno));
        return NULL;
    }
    loop->epollfd = epoll_create1(0);
    if (loop->epollfd == -1) {
        log_message(LOG_LEVEL_ERROR, "epoll_create1() failed: %s", strerror(errno));
        free(loop);
        return NULL;
    }
    loop->event_count = max_events;
    loop->events = (struct epoll_event *)malloc(sizeof(struct epoll_event) * max_events);
    if (!loop->events) {
        log_message(LOG_LEVEL_ERROR, "Failed to allocate memory for epoll events: %s", strerror(errno));
        close(loop->epollfd);
        free(loop);
        return NULL;
    }
    return loop;
}

int evnet_loop_add(EventLoop *loop,connection *conn) {
    struct epoll_event ev;
    ev.events = EPOLLIN ; 
    ev.data.ptr = conn;

    if (epoll_ctl(loop->epollfd, EPOLL_CTL_ADD, conn->fd, &ev) == -1) {
        log_message(LOG_LEVEL_ERROR, "epoll_ctl() failed to add fd %d: %s", conn->fd, strerror(errno));
        return -1;
    }
    return 1;
}

int event_loop_remove(EventLoop *loop, connection *conn) {
    if (epoll_ctl(loop->epollfd, EPOLL_CTL_DEL, conn->fd, NULL) == -1) {
        log_message(LOG_LEVEL_ERROR, "epoll_ctl() failed to remove fd %d: %s", conn->fd, strerror(errno));
        return -1;
    }
    return 1;
}

void event_loop_destroy(EventLoop *loop) {
    if (loop) {
        close(loop->epollfd);
        free(loop->events);
        free(loop);
    }
}
