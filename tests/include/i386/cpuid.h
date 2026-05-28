#ifndef MOCK_CPUID_H
#define MOCK_CPUID_H

#include <stdint.h>
#include <map>

#define eax 0
#define ebx 1
#define ecx 2
#define edx 3

struct cpuid_result {
    uint32_t regs[4];
};

extern std::map<uint32_t, cpuid_result> mock_cpuid_results;

static inline void cpuid(uint32_t *regs) {
    uint32_t leaf = regs[eax];
    if (mock_cpuid_results.count(leaf)) {
        regs[eax] = mock_cpuid_results[leaf].regs[eax];
        regs[ebx] = mock_cpuid_results[leaf].regs[ebx];
        regs[ecx] = mock_cpuid_results[leaf].regs[ecx];
        regs[edx] = mock_cpuid_results[leaf].regs[edx];
    } else {
        regs[eax] = 0;
        regs[ebx] = 0;
        regs[ecx] = 0;
        regs[edx] = 0;
    }
}

#endif
