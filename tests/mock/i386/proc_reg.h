#ifndef PROC_REG_H
#define PROC_REG_H

#include <stdint.h>

static inline uint64_t rdmsr64(uint32_t msr) { return 0; }
static inline void wrmsr64(uint32_t msr, uint64_t val) {}

#endif
