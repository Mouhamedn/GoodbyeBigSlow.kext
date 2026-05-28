#ifndef MOCK_LIBKERN_H
#define MOCK_LIBKERN_H

#include <stdint.h>
#include <string.h>

typedef int64_t SInt64;
typedef int32_t SInt32;

inline void OSIncrementAtomic64(volatile SInt64 *var) {
    __sync_fetch_and_add(var, 1);
}

#ifdef __cplusplus
extern "C" {
#endif
    bool PE_parse_boot_argn(const char *key, void *arg, int max_len);
#ifdef __cplusplus
}
#endif

#endif
