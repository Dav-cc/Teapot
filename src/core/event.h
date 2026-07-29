#ifndef __EVENT_H__
#define __EVENT_H__

#include <sys/epoll.h>
#include "../http/server.h"

#define EVENTS_SIZE 1024


#define EV_READABLE     2
#define EV_WRITABLE    1
#define EV_NULL        0 // not registerd events

typedef struct FileEvent{
    int fd;
    int mask;
    request_handler accept_func;
    request_handler write_func;
    request_handler read_func;
}FileEvent;

typedef struct FiredEvent {
    int fd;
    int flags;
    request_handler accept_func;
    request_handler write_func;
    request_handler read_func;
}FiredEvent;

typedef struct Eventstate{
    int epollfd;
    struct epoll_event events[EVENTS_SIZE];
}Eventstate;

typedef struct EventLoop {
    int running;
    int nevents;
    int setsize;
    Eventstate state;
    FiredEvent* fired;
    FileEvent* ev;
}EventLoop;

int EventLoop_ProcessEvents(EventLoop* el);
void RunEventLoop(EventLoop* el);
EventLoop* create_EventLoop(int events_size);
int EventLoop_DelEvent(EventLoop* el, int fd);
int EventLoop_ModEvent(EventLoop* el, int fd, int flags);
// int EventLoop_AddEvent(EventLoop* el, int fd, int flags, ReadHandler reader, WriteHandler writer, void* client_data);
int EventLoop_AddEvent(EventLoop* el, int fd, int flags,request_handler write_func, request_handler read_func,request_handler accept_func );
void EventLoop_Destroy(EventLoop* el);

#endif  // __EVENT_H__
