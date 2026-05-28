#ifndef MOCK_LIBKERN_H
#define MOCK_LIBKERN_H

#include <stdint.h>
#include <string.h>

typedef int64_t SInt64;

#define OSIncrementAtomic64(ptr) __sync_fetch_and_add((ptr), 1)

#endif
