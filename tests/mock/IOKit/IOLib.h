#ifndef MOCK_IOLIB_H
#define MOCK_IOLIB_H

#include <stdarg.h>
#include <stdio.h>

static inline void IOLog(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
}

static inline void IOSleep(uint32_t ms) {
    (void)ms;
}

#ifdef __cplusplus
extern "C" {
#endif

extern int PE_parse_boot_argn(const char *key, void *arg_ptr, int max_len);

#ifdef __cplusplus
}
#endif

#endif
