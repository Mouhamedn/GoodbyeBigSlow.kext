#ifndef MOCK_PROC_REG_H
#define MOCK_PROC_REG_H

#include <stdint.h>
#include <map>

extern std::map<uint32_t, uint64_t> mock_msrs;

static inline uint64_t rdmsr64(uint32_t msr) {
    if (mock_msrs.count(msr)) {
        return mock_msrs[msr];
    }
    return 0;
}

static inline void wrmsr64(uint32_t msr, uint64_t val) {
    mock_msrs[msr] = val;
}

#endif
