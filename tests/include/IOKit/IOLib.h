#ifndef IOLIB_H
#define IOLIB_H
#include <stdio.h>
#include <stdarg.h>
static inline void IOSleep(uint32_t ms) {}
static inline void IOLog(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
}
static inline bool PE_parse_boot_argn(const char *arg, void *ptr, int size) { return false; }
#endif
