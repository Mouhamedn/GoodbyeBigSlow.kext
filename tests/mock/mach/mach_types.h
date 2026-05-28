#ifndef MACH_TYPES_H
#define MACH_TYPES_H

#ifndef __unused
#define __unused __attribute__((unused))
#endif

typedef int kern_return_t;
#define KERN_SUCCESS 0
#define KERN_FAILURE 1

typedef struct kmod_info kmod_info_t;

#endif
