#include "log.h"
#include "event.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

EventLoop* create_EventLoop(int events_size){
    EventLoop* el = calloc(1,sizeof(EventLoop));
    if(!el){
        log_message(LOG_LEVEL_ERROR,"malloc faild : %s", strerror(errno));
        return NULL;
    }
    el->fired = calloc(events_size, sizeof(FiredEvent));
    if(el->fired == NULL){
        free(el);
        log_message(LOG_LEVEL_ERROR,"calloc faild : %s", strerror(errno));
        return NULL;
    }

    el->ev = calloc(events_size, sizeof(FileEvent));
    if(el->ev == NULL){
        free(el->fired);
        free(el);
        log_message(LOG_LEVEL_ERROR,"calloc faild : %s", strerror(errno));
        return NULL;
    }

    el->setsize = events_size;
    el->running = 0;

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
    int en, filed;
    int nevent = 0;
    while((nevent = epoll_wait(el->state.epollfd, el->state.events, el->setsize, -1)) == -1){
        if(nevent >= 0)break;
        if(nevent == -1){
            if(errno == EINTR){
                log_message(LOG_LEVEL_WARN, "recived EINTR signal, but event loop continues");
                if(!el->running) return 0;
                continue;
            }
            log_message(LOG_LEVEL_ERROR,"epoll wait returned -1 : %s", strerror(errno));
            return -1;
    }
    }
    en = nevent;
    for(int i = 0; i < en; i++){
        int flag = 0;
        struct epoll_event *event = el->state.events+i;
        if(event->events & EPOLLIN) flag |= EV_READABLE;
        if(event->events & EPOLLOUT) flag |= EV_WRITABLE;

        filed = event->data.fd;
        el->fired[i].flags = flag;
        el->fired[i].fd = filed;
        el->fired[i].acc = el->ev[filed].acc;
    }
    for(int j = 0 ; j < en; j++){
        FiredEvent* fe = el->fired +j;
        // IMPLEMENTED
        if((el->fired[j].flags & EV_READABLE )) fe->acc(fe->fd);
        // if((el->fired[j].flags & EV_WRITABLE)&& el->whandle) el->whandle(el->fired[j].fd, el->fired[j].flags);
    }
    return en;
}

int EventLoop_AddEvent(EventLoop* el, int fd, int flags,acceptor acc){
    struct epoll_event ee = {0};

    if(el->setsize <= fd ){ 
        log_message(LOG_LEVEL_ERROR, "given fd is bigger that fd set size");
        return -1;
    }
    FileEvent* fe = &el->ev[fd];
    fe->acc = acc;
    int op = el->ev[fd].mask == EV_NULL ? EPOLL_CTL_ADD : EPOLL_CTL_MOD;
    fe->mask |= flags;
    // fe->read_handler = reader;
    // fe->write_handler = writer;
    ee.data.fd = fd;

    if(flags & EV_READABLE){
        ee.events |= EPOLLIN;
        fe->acc = acc;
    }
    if(flags & EV_WRITABLE) ee.events |= EPOLLOUT;

    int res = epoll_ctl(el->state.epollfd, op, fd, &ee);
    if(res == -1){
        log_message(LOG_LEVEL_ERROR, "error in epoll_ctl : %s", strerror(errno));
        return -1;
    }
    return 0;
}

int EventLoop_ModEvent(EventLoop* el, int fd, int flags){
    struct epoll_event ee = {0};
    int op = EPOLL_CTL_MOD;

    if(el->nevents <= fd ) return -1;
    el->ev[fd].mask = flags;
    if(flags & EV_READABLE) ee.events |= EPOLLIN;
    if(flags & EV_WRITABLE) ee.events |= EPOLLOUT;
    ee.data.fd = fd;

    int res = epoll_ctl(el->state.epollfd, op, fd, &ee);
    if(res == -1){
        log_message(LOG_LEVEL_ERROR, "error in epoll_ctl : %s", strerror(errno));
        return -1;
    }
    return 0;
}


int EventLoop_DelEvent(EventLoop* el, int fd){
    int res = epoll_ctl(el->state.epollfd, EPOLL_CTL_DEL, fd, NULL);
    if(res == -1){
        log_message(LOG_LEVEL_ERROR, "error in epoll_ctl : %s", strerror(errno));
        return -1;
    }
    
    if(el->setsize <= fd ) {
            log_message(LOG_LEVEL_ERROR, "given fd is bigger that fd set size");
            return -1;
        }
    el->ev[fd].mask = EV_NULL;
    return 0;
}

void RunEventLoop(EventLoop* el){
    el->running = 1;
    log_message(LOG_LEVEL_INFO, "entring event loop . . . ");
    while(el->running){
        EventLoop_ProcessEvents(el);
    }
}

void EventLoop_Destroy(EventLoop* el) {
    if(!el) return ;
    if (el->ev) free(el->ev);
    if (el->fired) free(el->fired);
    if (el->state.epollfd >= 0) close(el->state.epollfd);
    free(el);
}
