#ifndef GOODBYE_BIG_SLOW_SHARED_H
#define GOODBYE_BIG_SLOW_SHARED_H

#include <stdint.h>

enum {
    kGBSMethodMSRRead = 0,
    kGBSMethodMSRWrite,
    kGBSMethodPLimit,
    kGBSMethodGPUFrequency,
    kGBSMethodCPUFrequency,
    kGBSMethodCPUIdle,
    kGBSMethodCPUHWP,
    kGBSMethodVoltage,
    kGBSMethodCount
};

enum {
    kGBSMSRModeSingle = 0,
    kGBSMSRModeAll = 1,
};

enum {
    kGBSMSROpWrite = 0,
    kGBSMSROpAnd = 1,
    kGBSMSROpOr = 2,
    kGBSMSROpXor = 3,
};

#endif
