#ifndef PROC_REG_H
#define PROC_REG_H

#include <stdint.h>

uint64_t rdmsr64(uint32_t msr);
void wrmsr64(uint32_t msr, uint64_t val);

#endif
