#ifndef I386_PROC_REG_H
#define I386_PROC_REG_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
uint64_t rdmsr64(uint32_t msr);
void wrmsr64(uint32_t msr, uint64_t val);
#ifdef __cplusplus
}
#endif

#endif
