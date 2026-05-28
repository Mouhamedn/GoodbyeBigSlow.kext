#ifndef I386_CPUID_H
#define I386_CPUID_H

#include <stdint.h>

enum {
    eax,
    ebx,
    ecx,
    edx
};

void cpuid(uint32_t *registers);

#endif
