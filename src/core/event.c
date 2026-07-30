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
        el->fired[i].accept_func = el->ev[filed].accept_func;
        el->fired[i].read_func = el->ev[filed].read_func;
        el->fired[i].write_func = el->ev[filed].write_func;
    }
    for(int j = 0 ; j < en; j++){
        FiredEvent* fe = el->fired +j;

        // IMPLEMENTED
        if((el->fired[j].flags & EV_READABLE )&& fe->read_func == NULL ) fe->accept_func(fe->fd, el);
        if((el->fired[j].flags & EV_READABLE )&& fe->read_func != NULL ) fe->read_func(fe->fd, el);
        if((el->fired[j].flags & EV_WRITABLE)) fe->write_func(fe->fd,el);
    }
    return en;
}

int EventLoop_AddEvent(EventLoop* el, int fd, int flags, request_handler write_func, request_handler read_func,request_handler accept_func ){
    struct epoll_event ee = {0};

    if(el->setsize <= fd ){ 
        log_message(LOG_LEVEL_ERROR, "given fd is bigger that fd set size");
        return -1;
    }
    FileEvent* fe = &el->ev[fd];
    if(read_func)
        fe->read_func = read_func;

    if(write_func)
        fe->write_func = write_func;

    if(accept_func)
        fe->accept_func = accept_func;
    // fe->accept_func = accept_func;
    // fe->read_func = read_func;
    // fe->write_func = write_func;
    int op = el->ev[fd].mask == EV_NULL ? EPOLL_CTL_ADD : EPOLL_CTL_MOD;
    fe->mask |= flags;
    ee.data.fd = fd;

    if(fe->mask & EV_READABLE){
        ee.events |= EPOLLIN;
        fe->accept_func = accept_func;
    }
    if(fe->mask & EV_WRITABLE) ee.events |= EPOLLOUT;

    int res = epoll_ctl(el->state.epollfd, op, fd, &ee);
    if(res == -1){
        log_message(LOG_LEVEL_ERROR, "error in epoll_ctl : %s", strerror(errno));
        return -1;
    }
    log_message(LOG_LEVEL_INFO,"ADD fd=%d",fd);
    return 0;
}

int EventLoop_ModEvent(EventLoop* el, int fd, int flag){
    struct epoll_event ee = {0};
    int op = EPOLL_CTL_MOD;

    if((el->setsize <= fd )&& fd > 0) return -1;
        el->ev[fd].mask |= flag;
    int mask = el->ev[fd].mask;
    if(mask & EV_READABLE) ee.events |= EPOLLIN;
    if(mask & EV_WRITABLE) ee.events |= EPOLLOUT;
    ee.data.fd = fd;
    log_message(LOG_LEVEL_INFO,"MOD fd=%d mask=%d",fd,mask);
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
    el->ev[fd].mask = EV_NULL;log_message(LOG_LEVEL_INFO,"DEL fd=%d",fd);
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
