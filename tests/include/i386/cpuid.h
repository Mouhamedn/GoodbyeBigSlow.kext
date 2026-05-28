#ifndef I386_CPUID_H
#define I386_CPUID_H

#include <stdint.h>

enum {
    eax,
    ebx,
    ecx,
    edx
};

#ifdef __cplusplus
extern "C" {
#endif
void cpuid(uint32_t *registers);
#ifdef __cplusplus
}
#endif

#endif
