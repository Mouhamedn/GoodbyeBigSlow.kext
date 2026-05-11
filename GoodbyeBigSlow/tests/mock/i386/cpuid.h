#ifndef CPUID_H
#define CPUID_H

#include <stdint.h>

enum {
    eax,
    ebx,
    ecx,
    edx
};

void cpuid(uint32_t *regs);

#endif
