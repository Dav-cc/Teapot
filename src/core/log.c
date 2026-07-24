#include "log.h"

LLevel log_level = LOG_LEVEL_DEBUG;

void log_message(LLevel level, char* format, ...) {
    if (log_level > level) {
        return;
    }

    va_list args;
    va_start(args, format);

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char time_str[20];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", t);

    printf("[%s] - ", time_str);
    vprintf(format, args);
    printf("\n");

    va_end(args);
}

