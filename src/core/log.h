#ifndef __LOG_H_
#define __LOG_H_

typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR
} LLevel;

void log_message(LLevel level, char* format, ...);
#endif //__LOG_H_
