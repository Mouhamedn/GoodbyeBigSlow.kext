#ifndef LIBKERN_H
#define LIBKERN_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

typedef int64_t SInt64;

static inline void OSIncrementAtomic64(volatile SInt64 *var) {
    __sync_fetch_and_add(var, 1);
}

#endif
