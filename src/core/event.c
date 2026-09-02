#include "log.h"
#include "event.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

EventLoop* eventloop_create(int max_fds){

    EventLoop* el = calloc(1,sizeof(EventLoop));
    if(!el){
        log_message(LOG_LEVEL_ERROR,"calloc failed for allocating el : %s", strerror(errno));
        return NULL;
    }
    el->state.events = calloc(max_fds, sizeof(struct epoll_event));
    if(!el->state.events){
        log_message(LOG_LEVEL_ERROR, "error in allocation");
        free(el);
        return NULL;
    }
    el->max_fds = max_fds;
    el->running = 1;

    el->state.epollfd = epoll_create1(0);
    if(el->state.epollfd == -1){
        log_message(LOG_LEVEL_ERROR,"epoll creation faild : %s", strerror(errno));
        free(el->ev);
        free(el);
        return NULL;
    }
    log_message(LOG_LEVEL_INFO, "EventLoop created");
    return el;
}

int eventloop_Process_events(EventLoop* el){
    int num_events = 0;
    // TODO: add TIMER support and ONESHOT
    num_events = epoll_wait(el->state.epollfd, el->state.events, el->max_fds, -1);
        if(num_events == -1){
            if(errno == EINTR){
                // TODO: implement gracefull shutdown
                log_message(LOG_LEVEL_WARN, "recived EINTR signal, stopping . . .\n");
                return -1;
            }

            log_message(LOG_LEVEL_ERROR,"epoll wait returned -1 : %s", strerror(errno));
            return -1;
        }
 
    for (int i = 0; i< num_events; i++){
        struct epoll_event *ee = &el->state.events[i];
        FileEvent* fe = ee->data.ptr;
        uint32_t flags = ee->events;

        if(flags & (EPOLLERR | EPOLLHUP)){
            if(fe->callbacks.on_error){
                fe->callbacks.on_error(el, fe);
            }
            continue;
        }
        if(flags & EPOLLIN){
            if(fe->callbacks.on_read){
                fe->callbacks.on_read(el, fe);
            }
        }
        if(flags & EPOLLOUT){
            if(fe->callbacks.on_write){
                fe->callbacks.on_write(el, fe);
            }
        }
    }
    return num_events;
}

int eventloop_add_event(EventLoop* el, FileEvent* fe){
    if(fe->fd >= el->max_fds){
        log_message(LOG_LEVEL_ERROR, "given fd is bigger than max fd");
        return -1;
    }
    struct epoll_event ee = {0};
    ee.data.ptr = fe;

    if (fe->mask & EV_ET)
        ee.events |= EPOLLET;

    if (fe->mask & EV_READABLE)
        ee.events |= EPOLLIN;

    if (fe->mask & EV_WRITABLE)
        ee.events |= EPOLLOUT;

    if((epoll_ctl(el->state.epollfd,EPOLL_CTL_ADD, fe->fd,&ee)) == -1){
        log_message(LOG_LEVEL_ERROR, "Falied to ADD fd to epoll");
        return -1;
    }
    return 0;
}

int eventloop_mod_event(EventLoop* el, FileEvent* fe, uint32_t flag){
    struct epoll_event ee= {0};
    ee.data.ptr = fe;
    ee.events = 0;

    if(flag & EV_READABLE){
        ee.events |= EPOLLIN;
    }
    if(flag & EV_WRITABLE){
        ee.events |= EPOLLOUT;
    }
    return epoll_ctl(el->state.epollfd, EPOLL_CTL_MOD, fe->fd, &ee);
}

int eventloop_del_event(EventLoop* el, FileEvent* fe){
    int res = epoll_ctl(el->state.epollfd, EPOLL_CTL_DEL, fe->fd, NULL);
    if(res == -1){
        log_message(LOG_LEVEL_ERROR, "error in deleting fe->fd:%d : %s",fe->fd, strerror(errno));
        return -1;
    }
    return 0;
}

void eventloop_run(EventLoop* el){
    el->running = 1;
    log_message(LOG_LEVEL_INFO, "Event Loop Started. . . ");
    while(el->running){
        eventloop_Process_events(el);
    }
}

void eventloop_destroy(EventLoop* el) {
    if(!el) return ;
    if (el->ev) free(el->ev);
    if (el->state.epollfd >= 0) close(el->state.epollfd);
    free(el);
}
