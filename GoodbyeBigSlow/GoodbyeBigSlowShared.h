#ifndef GoodbyeBigSlowShared_h
#define GoodbyeBigSlowShared_h

#include <stdint.h>

enum {
    kGoodbyeBigSlowReadMSR,
    kGoodbyeBigSlowWriteMSR,
    kGoodbyeBigSlowNumberOfMethods
};

typedef struct {
    uint32_t index;
    uint64_t value;
} GoodbyeBigSlowMSRArgs;

#ifndef MSR_IA32_PLATFORM_ID
#define MSR_IA32_PLATFORM_ID 0x17
#endif
#ifndef MSR_IA32_PERF_STS
#define MSR_IA32_PERF_STS 0x198
#endif
#ifndef MSR_IA32_PERF_CTL
#define MSR_IA32_PERF_CTL 0x199
#endif
#ifndef MSR_IA32_THERM_STATUS
#define MSR_IA32_THERM_STATUS 0x19C
#endif
#ifndef MSR_IA32_MISC_ENABLE
#define MSR_IA32_MISC_ENABLE 0x1A0
#endif
#ifndef MSR_IA32_PACKAGE_THERM_STATUS
#define MSR_IA32_PACKAGE_THERM_STATUS 0x1B1
#endif
#ifndef MSR_IA32_POWER_CTL
#define MSR_IA32_POWER_CTL 0x1FC
#endif

#endif
