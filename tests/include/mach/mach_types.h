#ifndef MACH_MACH_TYPES_H
#define MACH_MACH_TYPES_H

#include <stdint.h>

typedef int kern_return_t;
#define KERN_SUCCESS 0
#define KERN_FAILURE 1

typedef struct kmod_info {
    int dummy;
} kmod_info_t;

typedef kern_return_t kmod_start_func_t(kmod_info_t *ki, void *data);
typedef kern_return_t kmod_stop_func_t(kmod_info_t *ki, void *data);

#define KMOD_EXPLICIT_DECL(name, ver, start, stop)

#endif
