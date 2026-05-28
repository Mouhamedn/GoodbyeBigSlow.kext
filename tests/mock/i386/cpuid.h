#ifndef MOCK_CPUID_H
#define MOCK_CPUID_H

#include <stdint.h>

#define eax 0
#define ebx 1
#define ecx 2
#define edx 3

#ifdef __cplusplus
extern "C" {
#endif

extern void cpuid(uint32_t *regs);

#ifdef __cplusplus
}
#endif

#endif
