#ifndef MOCK_PROC_REG_H
#define MOCK_PROC_REG_H

#include <stdint.h>

extern uint64_t rdmsr64(uint32_t msr);
extern void wrmsr64(uint32_t msr, uint64_t val);

#endif
