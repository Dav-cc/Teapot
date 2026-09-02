#ifndef TEVENTLOOP_H
#define TEVENTLOOP_H
#include <stdint.h>
#include <sys/epoll.h>


#define EV_READABLE   (1 << 0)
#define EV_WRITABLE   (1 << 1)
#define EV_ET         (1 << 2)


typedef struct EventLoop EventLoop;
typedef struct FileEvent FileEvent;


typedef int (*event_callback)(EventLoop *loop, FileEvent *event);

typedef struct Event_Callback{
    event_callback on_read;
    event_callback on_write;
    event_callback on_error;
}Event_Callback;

typedef struct FileEvent{
    int fd;         // this filed used for file descriptore
                    // and in default value it's -1 on init

    uint32_t mask;   // 32 bit mask for tracking events state like (read/writabl)

    Event_Callback callbacks;

    void* data;     // this well be used for keeping events object (like Connection
                    // for client/server sockets, or timer fd)
}FileEvent;

typedef struct Eventstate{
    int epollfd;
    struct epoll_event *events;  // allocation in eventloop_create based on max_event_set
}Epoll_Watcher;

typedef struct EventLoop {
    int running;             // if event loop running 1 if stopped 0

    int max_fds;             // maxmun number of events that we track with epoll

    Epoll_Watcher state;     // holds epoll things

    FileEvent* ev;           // max number of ev's we created based on max_event_set
}EventLoop;

typedef enum EventError{
    LOOP_OK = 0, 
    LOOP_NO_SPACE = -1,
    LOOP_EPOLL_ERROR = -2,
    LOOP_FD_NOT_VALID = -3,
    LOOP_CTL_ERROR = -4,
    LOOP_SIGNAL = -5,
}EventError;

EventError eventloop_process_events(EventLoop* el);
void eventloop_run(EventLoop* el);
EventLoop* eventloop_create(int max_fds);
EventError eventloop_del_event(EventLoop* el, FileEvent* fe);
EventError eventloop_mod_event(EventLoop* el, FileEvent* fe, uint32_t flags);
EventError eventloop_add_event(EventLoop* el, FileEvent* fe);
void eventLoop_destroy(EventLoop* el);
#endif  //TEVENTLOOP_H
