#ifndef IOKIT_IOLIB_H
#define IOKIT_IOLIB_H

#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

static void IOLog(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
}

static void IOSleep(uint32_t milliseconds) {
    // Dummy
}

static bool PE_parse_boot_argn(const char *argName, void *argPtr, int argSize) {
    return false;
}

#endif
