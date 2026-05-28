#ifndef LIBKERN_H
#define LIBKERN_H
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

typedef int64_t SInt64;
static inline void OSIncrementAtomic64(volatile SInt64 *var) {
    __sync_fetch_and_add(var, 1);
}

#define kern_return_t int
#define KERN_SUCCESS 0
#define KERN_FAILURE 1

static inline void mp_rendezvous_no_intrs(void (*func)(void *), void *arg) {}

#endif
