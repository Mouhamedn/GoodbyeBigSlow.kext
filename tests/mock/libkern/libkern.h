#ifndef MOCK_LIBKERN_H
#define MOCK_LIBKERN_H

#include <stdint.h>
#include <string.h>

typedef int64_t SInt64;
typedef int32_t SInt32;
typedef uint32_t UInt32;
typedef int kern_return_t;

#define KERN_SUCCESS 0
#define KERN_FAILURE 1

static inline int64_t OSIncrementAtomic64(volatile int64_t *var) {
    return __sync_fetch_and_add(var, 1);
}

#endif
