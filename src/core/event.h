#ifndef __EVENT_H__
#define __EVENT_H__

#include <sys/epoll.h>
#include "../http/server.h"

#define EVENTS_SIZE 1024

#define EV_EPOLLRDHUP  4
#define EV_ERROR       3
#define EV_READABLE    2
#define EV_WRITABLE    1
#define EV_NULL        0 // not registerd events

typedef struct FileEvent{
    int fd;
    int mask;
    Connection* conn;  // handlers moved in this struct
    // connection_handler accept_func;
    // connection_handler write_func;
    // connection_handler read_func;
}FileEvent;

typedef struct FiredEvent {
    int fd;
    int flags;
    Connection* conn;  // handlers moved in this struct
    // connection_handler accept_func;
    // connection_handler write_func;
    // connection_handler read_func;
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
int EventLoop_DelEvent(EventLoop* el, Connection* conn);
int EventLoop_ModEvent(EventLoop* el, Connection* conn, int flags);
int EventLoop_AddEvent(EventLoop* el, Connection* conn, int flags);
void EventLoop_Destroy(EventLoop* el);

#endif  // __EVENT_H__
