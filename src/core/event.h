#ifndef __EVENT_H__
#define __EVENT_H__

#include <sys/epoll.h>

typedef enum Eventkind{
    EVENT_CONNECTION,
    EVENT_LISTENER,
    // EVENT_SIGNAL,
    EVENT_TIMER,
} Eventkind;

typedef struct EventType{
    int Eventfd;
    Eventkind type;
}EventType;

typedef struct EventLoop{
    int is_running;
    int epollfd;
    int event_count;
    struct epoll_event *events;
}EventLoop;


typedef struct connection{
    int fd;
    EventType type;
    char *buffer_read;
    char *buffer_write;
    int buffer_read_size;
    int buffer_write_size;
}connection;



#endif  // __EVENT_H__
