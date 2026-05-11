#ifndef LIBKERN_H
#define LIBKERN_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

typedef int64_t SInt64;

void OSIncrementAtomic64(volatile SInt64 *var);

#ifndef __unused
#define __unused __attribute__((unused))
#endif

#endif
