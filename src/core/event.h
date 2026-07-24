#ifndef __EVENT_H__
#define __EVENT_H__

#include <sys/epoll.h>

#define EVENTS_SIZE 1024


#define FD_REDABLE     2
#define FD_WRITABLE    1
#define FD_NULL        0 // not registerd events

typedef void(*AcceptHandler)(int, int);
typedef void(*ReadHandler)(int, int);
typedef void(*WriteHandler)(int, int);

typedef struct Event{
    int fd;
    int mask;
    ReadHandler rhandle;
    WriteHandler whandle;
}Events;
typedef struct FiredEvents {
    int fd;
    int flags;
}FiredEvents;

typedef struct Eventstate{
    int epollfd;
    struct epoll_event events[EVENTS_SIZE];
}Eventstate;

typedef struct EventLoop {
    int ruuning;
    int nevents;
    int setsize;
    AcceptHandler handler;
    ReadHandler rhandle;
    WriteHandler whandle;
    Eventstate state;
    FiredEvents* fired;
    Events* ev;
}EventLoop;

int EventLoop_ProcessEvents(EventLoop* el);
void RunEventLoop(EventLoop* el);
EventLoop* create_EventLoop(int events_size, AcceptHandler handler, ReadHandler read, WriteHandler write);
int EventLoop_DelFd(EventLoop* el, int fd);
int EventLoop_ModFd(EventLoop* el, int fd, int flags);
int EventLoop_AddFd(EventLoop* el, int fd, int flags);

#endif  // __EVENT_H__
