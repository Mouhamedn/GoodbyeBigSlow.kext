#ifndef MOCK_MACH_TYPES_H
#define MOCK_MACH_TYPES_H

#include <stdint.h>

typedef void* kmod_info_t;
typedef void* void_t;
typedef uint32_t task_t;

#define __unused __attribute__((unused))
#define __APPLE_CC__ 1

#endif
