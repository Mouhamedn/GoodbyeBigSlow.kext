#include <IOKit/IOKitLib.h>
#include <CoreFoundation/CoreFoundation.h>
#include <stdio.h>
#include <string.h>
#include "GoodbyeBigSlowShared.h"

#ifndef kIOMasterPortDefault
#define kIOMasterPortDefault kIOMainPortDefault
#endif

static void print_msr(uint32_t index, uint64_t value)
{
    const char* name = "UNKNOWN";
    switch (index) {
        case MSR_IA32_PLATFORM_ID: name = "PLATFORM_ID"; break;
        case MSR_IA32_PERF_STS: name = "PERF_STS (PLIMIT)"; break;
        case MSR_IA32_PERF_CTL: name = "PERF_CTL"; break;
        case MSR_IA32_THERM_STATUS: name = "THERM_STATUS"; break;
        case MSR_IA32_MISC_ENABLE: name = "MISC_ENABLE"; break;
        case MSR_IA32_PACKAGE_THERM_STATUS: name = "PACKAGE_THERM_STATUS"; break;
        case MSR_IA32_POWER_CTL: name = "POWER_CTL"; break;
    }
    printf("MSR 0x%04X (%s): 0x%016llX\n", index, name, value);
}

int main(int argc, char* argv[])
{
    io_service_t service = IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceMatching("GoodbyeBigSlow"));
    if (!service) {
        fprintf(stderr, "GoodbyeBigSlow service not found\n");
        return 1;
    }

    io_connect_t connect;
    kern_return_t kr = IOServiceOpen(service, mach_task_self(), 0, &connect);
    IOObjectRelease(service);

    if (kr != KERN_SUCCESS) {
        fprintf(stderr, "Failed to open service: 0x%08x\n", kr);
        return 1;
    }

    if (argc > 1 && strcmp(argv[1], "plimit") == 0) {
        GoodbyeBigSlowMSRArgs input = { MSR_IA32_PERF_STS, 0 };
        GoodbyeBigSlowMSRArgs output = { 0, 0 };
        size_t outputSize = sizeof(output);

        kr = IOConnectCallStructMethod(connect, kGoodbyeBigSlowReadMSR, &input, sizeof(input), &output, &outputSize);
        if (kr == KERN_SUCCESS) {
            print_msr(output.index, output.value);
        } else {
            fprintf(stderr, "Failed to read MSR: 0x%08x\n", kr);
        }
    } else {
        uint32_t indices[] = {
            MSR_IA32_PLATFORM_ID,
            MSR_IA32_PERF_STS,
            MSR_IA32_PERF_CTL,
            MSR_IA32_THERM_STATUS,
            MSR_IA32_MISC_ENABLE,
            MSR_IA32_PACKAGE_THERM_STATUS,
            MSR_IA32_POWER_CTL
        };

        for (int i = 0; i < sizeof(indices) / sizeof(indices[0]); i++) {
            GoodbyeBigSlowMSRArgs input = { indices[i], 0 };
            GoodbyeBigSlowMSRArgs output = { 0, 0 };
            size_t outputSize = sizeof(output);

            kr = IOConnectCallStructMethod(connect, kGoodbyeBigSlowReadMSR, &input, sizeof(input), &output, &outputSize);
            if (kr == KERN_SUCCESS) {
                print_msr(output.index, output.value);
            }
        }
    }

    IOServiceClose(connect);
    return 0;
}
