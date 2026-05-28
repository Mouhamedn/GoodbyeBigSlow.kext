#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <IOKit/IOKitLib.h>

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

#define SERVICE_NAME "GoodbyeBigSlow"

#ifndef kIOMainPortDefault
#define kIOMainPortDefault kIOMasterPortDefault
#endif

static uint64_t read_msr(io_connect_t connect, uint32_t msr) {
    uint64_t input[1] = {msr};
    uint64_t output[1];
    uint32_t output_count = 1;
    kern_return_t kr = IOConnectCallScalarMethod(connect, 0, input, 1, output, &output_count);
    if (kr != KERN_SUCCESS) {
        fprintf(stderr, "Error reading MSR 0x%X: 0x%08x\n", msr, kr);
        return 0;
    }
    return output[0];
}

static void write_msr(io_connect_t connect, uint32_t msr, uint64_t value) {
    uint64_t input[2] = {msr, value};
    kern_return_t kr = IOConnectCallScalarMethod(connect, 1, input, 2, NULL, NULL);
    if (kr != KERN_SUCCESS) {
        fprintf(stderr, "Error writing MSR 0x%X: 0x%08x\n", msr, kr);
    }
}

static void print_status(io_connect_t connect) {
    uint64_t platform_id = read_msr(connect, MSR_IA32_PLATFORM_ID);
    uint64_t perf_sts = read_msr(connect, MSR_IA32_PERF_STS);
    uint64_t perf_ctl = read_msr(connect, MSR_IA32_PERF_CTL);
    uint64_t therm_status = read_msr(connect, MSR_IA32_THERM_STATUS);
    uint64_t misc_enable = read_msr(connect, MSR_IA32_MISC_ENABLE);
    uint64_t pkg_therm = read_msr(connect, MSR_IA32_PACKAGE_THERM_STATUS);
    uint64_t power_ctl = read_msr(connect, MSR_IA32_POWER_CTL);

    printf("GoodbyeBigSlow Status:\n");
    printf("  MSR_IA32_PLATFORM_ID:         0x%016llx\n", platform_id);
    printf("  MSR_IA32_PERF_STS (PLIMIT):   0x%016llx\n", perf_sts);
    printf("  MSR_IA32_PERF_CTL:            0x%016llx\n", perf_ctl);
    printf("  MSR_IA32_THERM_STATUS:        0x%016llx\n", therm_status);
    printf("  MSR_IA32_MISC_ENABLE:         0x%016llx\n", misc_enable);
    printf("  MSR_IA32_PACKAGE_THERM_STATUS:0x%016llx\n", pkg_therm);
    printf("  MSR_IA32_POWER_CTL:           0x%016llx\n", power_ctl);
}

int main(int argc, char *argv[]) {
    io_service_t service = IOServiceGetMatchingService(kIOMainPortDefault, IOServiceMatching(SERVICE_NAME));
    if (!service) {
        fprintf(stderr, "Service %s not found\n", SERVICE_NAME);
        return 1;
    }

    io_connect_t connect;
    kern_return_t kr = IOServiceOpen(service, mach_task_self(), 0, &connect);
    IOObjectRelease(service);

    if (kr != KERN_SUCCESS) {
        fprintf(stderr, "Could not open service: 0x%08x\n", kr);
        return 1;
    }

    if (argc == 1) {
        print_status(connect);
    } else if (strcmp(argv[1], "plimit") == 0) {
        if (argc == 3) {
            uint64_t val = strtoull(argv[2], NULL, 0);
            write_msr(connect, MSR_IA32_PERF_CTL, (val & 0xFF) << 8);
        } else {
            uint64_t val = read_msr(connect, MSR_IA32_PERF_STS);
            printf("PLIMIT: %llu\n", (val >> 8) & 0xFF);
        }
    } else if (strcmp(argv[1], "cpu") == 0 && argc >= 3) {
        if (strcmp(argv[2], "frequency") == 0) {
            if (argc == 4) {
                uint64_t val = strtoull(argv[3], NULL, 0);
                write_msr(connect, MSR_IA32_PERF_CTL, (val & 0xFF) << 8);
            } else {
                uint64_t val = read_msr(connect, MSR_IA32_PERF_STS);
                printf("CPU Frequency: %llu\n", (val >> 8) & 0xFF);
            }
        } else if (strcmp(argv[2], "hwp") == 0) {
             // 0x770 IA32_PM_ENABLE, 0x774 IA32_HWP_REQUEST
             // For now just placeholders or simple implementations if MSRs are whitelisted
             printf("HWP command not fully implemented yet\n");
        }
    } else if (strcmp(argv[1], "rdmsr") == 0 && argc == 3) {
        uint32_t msr = (uint32_t)strtoul(argv[2], NULL, 0);
        printf("0x%X: 0x%llx\n", msr, read_msr(connect, msr));
    } else if (strcmp(argv[1], "wrmsr") == 0 && argc == 4) {
        uint32_t msr = (uint32_t)strtoul(argv[2], NULL, 0);
        uint64_t val = strtoull(argv[3], NULL, 0);
        write_msr(connect, msr, val);
    } else {
        printf("Usage: %s [plimit|cpu frequency|rdmsr|wrmsr] ...\n", argv[0]);
    }

    IOServiceClose(connect);
    return 0;
}
