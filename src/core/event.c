#include "log.h"
#include "event.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
// #include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

EventLoop* create_EventLoop(int events_size, AcceptHandler handler, ReadHandler read, WriteHandler write){
    EventLoop* el = calloc(1,sizeof(EventLoop));
    if(!el){
        log_message(LOG_LEVEL_ERROR,"malloc faild : %s", strerror(errno));
        return NULL;
    }
    el->fired = calloc(events_size, sizeof(FiredEvents));
    if(el->fired == NULL){
        free(el);
        log_message(LOG_LEVEL_ERROR,"calloc faild : %s", strerror(errno));
        return NULL;
    }

    el->ev = calloc(events_size, sizeof(Events));
    if(el->ev == NULL){
        free(el->fired);
        free(el);
        log_message(LOG_LEVEL_ERROR,"calloc faild : %s", strerror(errno));
        return NULL;
    }

    el->setsize = events_size;
    el->handler = handler;
    el->rhandle = read;
    el->whandle = write;
    el->ruuning = 0;

    el->state.epollfd = epoll_create1(0);
    if(el->state.epollfd == -1){
        log_message(LOG_LEVEL_ERROR,"epoll create faild : %s", strerror(errno));
        free(el->ev);
        free(el->fired);
        free(el);
        return NULL;
    }

    log_message(LOG_LEVEL_INFO, "EventLoop created");
    return el;
}

int EventLoop_ProcessEvents(EventLoop* el){
    int en;
    int nevent = epoll_wait(el->state.epollfd, el->state.events, el->setsize, -1);
    if(nevent == -1){
        log_message(LOG_LEVEL_ERROR,"epoll wait returned -1 : %s", strerror(errno));
        return -1;
    }
    en = nevent;
    for(int i = 0; i < en; i++){
        int flag = 0;
        struct epoll_event *event = el->state.events+i;
        if(event->events & EPOLLIN) flag |= FD_REDABLE;
        if(event->events & EPOLLOUT) flag |= FD_WRITABLE;
        el->fired[i].fd = event->data.fd;
        el->fired[i].flags = flag;
    }
    for(int j = 0 ; j < en; j++){
        printf("im here . . . .\n");
        if((el->fired[j].flags & FD_REDABLE )&& el->rhandle) el->handler(el->fired[j].fd, el->fired[j].flags);
        if((el->fired[j].flags & FD_WRITABLE)&& el->whandle) el->whandle(el->fired[j].fd, el->fired[j].flags);
    }
    return en;
}

int EventLoop_AddFd(EventLoop* el, int fd, int flags){
    struct epoll_event ee = {0};
    int op = el->ev[fd].mask == FD_NULL ? EPOLL_CTL_ADD : EPOLL_CTL_MOD;
    flags |= el->ev[fd].mask;
    el->ev[fd].mask = flags;
    if(flags & FD_REDABLE) ee.events |= EPOLLIN;
    if(flags & FD_WRITABLE) ee.events |= EPOLLOUT;
    ee.data.fd = fd;
    int res = epoll_ctl(el->state.epollfd, op, fd, &ee);
    if(res == -1){
        log_message(LOG_LEVEL_ERROR, "error in epoll_ctl : %s", strerror(errno));
        return -1;
    }
    return 0;
}

int EventLoop_ModFd(EventLoop* el, int fd, int flags){
    struct epoll_event ee = {0};
    int op = EPOLL_CTL_MOD;
    el->ev[fd].mask = 0;
    el->ev[fd].mask = flags;
    if(flags & FD_REDABLE) ee.events |= EPOLLIN;
    if(flags & FD_WRITABLE) ee.events |= EPOLLOUT;
    ee.data.fd = fd;

    int res = epoll_ctl(el->state.epollfd, op, fd, &ee);
    if(res == -1){
        log_message(LOG_LEVEL_ERROR, "error in epoll_ctl : %s", strerror(errno));
        return -1;
    }
    return 0;
}


int EventLoop_DelFd(EventLoop* el, int fd){
    int res = epoll_ctl(el->state.epollfd, EPOLL_CTL_DEL, fd, NULL);
    if(res == -1){
        log_message(LOG_LEVEL_ERROR, "error in epoll_ctl : %s", strerror(errno));
        return -1;
    }
    el->ev[fd].mask = FD_NULL;
    return 0;
}

void RunEventLoop(EventLoop* el){
    el->ruuning = 1;
    log_message(LOG_LEVEL_INFO, "entring event loop . . . ");
    while(el->ruuning){
        EventLoop_ProcessEvents(el);
    }
}

void EventLoop_Distroy(EventLoop* el){
    if (!el->ev || !el->fired){
    free(el->ev);
    free(el->fired);
    free(el);
    }
}
