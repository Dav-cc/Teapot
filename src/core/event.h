#ifndef __EVENT_H__
#define __EVENT_H__

#include <sys/epoll.h>
#include "../http/server.h"

#define EVENTS_SIZE 1024

#define EV_EPOLLRDHUP  4
#define EV_ERROR       8
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

int eventloop_process_events(EventLoop* el);
void eventloop_run(EventLoop* el);
EventLoop* eventloop_create(int events_size);
int eventloop_del_event(EventLoop* el, Connection* conn);
int eventloop_mod_event(EventLoop* el, Connection* conn, int flags);
int eventloop_add_event(EventLoop* el, Connection* conn, int flags);
void eventLoop_destroy(EventLoop* el);

#endif  // __EVENT_H__
