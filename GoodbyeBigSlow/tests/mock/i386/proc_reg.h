#ifndef I386_PROC_REG_H
#define I386_PROC_REG_H

#include <stdint.h>

static uint64_t rdmsr64(uint32_t msr) { return 0; }
static void wrmsr64(uint32_t msr, uint64_t val) {}

#endif
