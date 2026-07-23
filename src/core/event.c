#include "log.h"
#include "event.h"
#include <errno.h>
#include <stdlib.h>
// #include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

EventLoop* create_EventLoop(AcceptHandler handler){
    EventLoop* el = malloc(sizeof(EventLoop));
    if(!el){
        log_message(LOG_LEVEL_ERROR,"malloc faild : %s", strerror(errno));
        return NULL;
    }
    el->handler = handler;
    el->ruuning = 0;
    el->state.epollfd = epoll_create1(0);
    if(el->state.epollfd == -1){
        log_message(LOG_LEVEL_ERROR,"epoll create faild : %s", strerror(errno));
        free(el);
        return NULL;
    }
    log_message(LOG_LEVEL_INFO, "EventLoop created");
    return el;
}

int EventLoop_ProcessEvents(EventLoop* el){
    int en;
    el->nevents = epoll_wait(el->state.epollfd, el->state.events, 1024, -1);
    if(el->nevents == -1){
        log_message(LOG_LEVEL_ERROR,"epoll wait returned -1 : %s", strerror(errno));
        return -1;
    }
    en = el->nevents;
    for(int i = 0; i < en; i++){
        int flag = 0;
        struct epoll_event *event = el->state.events+i;
        if(event->events & EPOLLIN) flag |= FD_REDABLE;
        if(event->events & EPOLLOUT) flag |= FD_WRITABLE;
        el->fired[i].fd = event->data.fd;
        el->fired[i].flags = flag;
    }
    for(int j = 0 ; j < en; j++){
        if(el->fired[j].flags & FD_REDABLE) el->rhandle(el->fired[j].fd, el->fired[j].flags);
        if(el->fired[j].flags & FD_WRITABLE) el->whandle(el->fired[j].fd, el->fired[j].flags);
    }
    // implemented
}

void RunEventLoop(EventLoop* el){
    el->ruuning = 1;
    while(el->ruuning){
        EventLoop_ProcessEvents(el);
    }
}

void EventLoop_Distroy(EventLoop* el){
    free(el);
}
