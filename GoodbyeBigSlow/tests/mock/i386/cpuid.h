#ifndef I386_CPUID_H
#define I386_CPUID_H

#include <stdint.h>

enum {
    eax = 0,
    ebx = 1,
    ecx = 2,
    edx = 3
};

static void cpuid(uint32_t *regs) {
    // Dummy implementation
}

#endif
