#ifndef MOCK_IOLIB_H
#define MOCK_IOLIB_H

#include <stdio.h>
#include <stdarg.h>

static inline void IODelay(unsigned int microseconds) { (void)microseconds; }
static inline void IOSleep(unsigned int milliseconds) { (void)milliseconds; }
static inline void IOLog(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

#define KMOD_EXPLICIT_DECL(id, version, start, stop)
#define __private_extern__ extern

#endif
