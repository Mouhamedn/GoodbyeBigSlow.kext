#ifndef MACH_TYPES_H
#define MACH_TYPES_H

#include <stdint.h>

typedef int kern_return_t;
#define KERN_SUCCESS 0
#define KERN_FAILURE 1

typedef struct kmod_info {
    // dummy
} kmod_info_t;

#define KMOD_EXPLICIT_DECL(id, version, start, stop)

#endif
