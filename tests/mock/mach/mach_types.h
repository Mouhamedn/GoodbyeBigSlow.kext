#ifndef MOCK_MACH_TYPES_H
#define MOCK_MACH_TYPES_H

#include <stdint.h>

typedef int32_t kern_return_t;
#define KERN_SUCCESS 0
#define KERN_FAILURE 1

typedef void kmod_info_t;

#ifndef __unused
#define __unused __attribute__((unused))
#endif

#ifndef __APPLE_CC__
#define __APPLE_CC__ 1
#endif

typedef void (*kmod_start_func_t)(kmod_info_t *, void *);
typedef void (*kmod_stop_func_t)(kmod_info_t *, void *);

#endif
