#ifndef LIBKERN_LIBKERN_H
#define LIBKERN_LIBKERN_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

typedef int32_t kern_return_t;
#define KERN_SUCCESS 0
#define KERN_FAILURE 1

#ifndef __unused
#define __unused __attribute__((unused))
#endif

typedef int64_t SInt64;

static void OSIncrementAtomic64(volatile SInt64 *var) {
    __sync_fetch_and_add(var, 1);
}

#endif
