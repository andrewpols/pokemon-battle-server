#ifndef _LOG_H_
#define _LOG_H_

#include <stdio.h>
#include <time.h>
#include <stdarg.h>

static inline void log_msg(const char *fmt, ...) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    printf("[%02d:%02d:%02d] ", tm->tm_hour, tm->tm_min, tm->tm_sec);
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
    fflush(stdout);
}

#endif
