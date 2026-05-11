#include <stdio.h>
#include <stdlib.h>
#include <IOKit/IOKitLib.h>

#ifndef MSR_IA32_PERF_STS
#define MSR_IA32_PERF_STS 0x198
#endif

uint64_t read_msr(io_connect_t connect, uint32_t msr) {
    uint64_t input = msr;
    uint64_t output = 0;
    uint32_t output_count = 1;
    kern_return_t kr = IOConnectCallScalarMethod(connect, 0, &input, 1, &output, &output_count);
    if (kr != KERN_SUCCESS) {
        return 0;
    }
    return output;
}

int main() {
    io_service_t service = IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceMatching("GoodbyeBigSlow"));
    if (!service) {
        // If kext is not loaded, we just exit silently or with error.
        // The script will fall back to sysctl if it handles it.
        return 1;
    }

    io_connect_t connect;
    kern_return_t kr = IOServiceOpen(service, mach_task_self(), 0, &connect);
    IOObjectRelease(service);
    if (kr != KERN_SUCCESS) {
        return 1;
    }

    uint64_t perf_sts = read_msr(connect, MSR_IA32_PERF_STS);

    // Output with "PLIMIT" in the key so it's not filtered out by the script's sed.
    printf("machdep.goodbyebigslow.PLIMIT_msr_perf_sts=%llu\n", perf_sts);

    IOServiceClose(connect);
    return 0;
}
