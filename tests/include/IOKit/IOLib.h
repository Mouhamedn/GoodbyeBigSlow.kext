#ifndef IOKIT_IOLIB_H
#define IOKIT_IOLIB_H

#include <stdint.h>

void IOLog(const char *fmt, ...);
void IOSleep(uint32_t milliseconds);

void OSIncrementAtomic64(volatile int64_t *dst);

#ifndef __unused
#define __unused __attribute__((unused))
#endif

#endif
