#ifndef __EVENT_H__
#define __EVENT_H__

#include <sys/epoll.h>

#define EVENTS_SIZE 1024


#define FD_REDABLE (1<<1)
#define FD_WRITABLE (1<<2)

typedef void(*AcceptHandler)(int, int);
typedef void(*ReadHandler)(int, int);
typedef void(*WriteHandler)(int, int);

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
    AcceptHandler handler;
    ReadHandler rhandle;
    WriteHandler whandle;
    Eventstate state;
    FiredEvents fired[1024];
}EventLoop;

int EventLoop_ProcessEvents(EventLoop* el);
void RunEventLoop(EventLoop* el);
EventLoop* create_EventLoop(AcceptHandler handler);

#endif  // __EVENT_H__
