#ifndef MOCK_IOLIB_H
#define MOCK_IOLIB_H

#include <stdio.h>
#include <stdarg.h>

inline void IOLog(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
}

#ifndef __unused
#define __unused __attribute__((unused))
#endif

#define KERN_SUCCESS 0
#define KERN_FAILURE 1
typedef int kern_return_t;

inline void IOSleep(unsigned int ms) {
    // mock sleep
}

#endif
