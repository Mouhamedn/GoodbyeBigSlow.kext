#include <IOKit/IOKitLib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "GoodbyeBigSlowShared.h"

void usage(const char *prog) {
    printf("Usage: %s <command> [args...]\n", prog);
    printf("Commands:\n");
    printf("  plimit [<frequency>]\n");
    printf("  gpu frequency [<frequency>]\n");
    printf("  cpu frequency [<frequency>]\n");
    printf("  cpu idle [<percent>]\n");
    printf("  cpu hwp [<frequency>]\n");
    printf("  voltage <min> <max>\n");
    printf("  msr <address> [[and|or|xor] <value>]\n");
    printf("  msr_mp <address> [[and|or|xor] <value>]\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        usage(argv[0]);
        return 1;
    }

#ifndef kIOMainPortDefault
#define kIOMainPortDefault kIOMasterPortDefault
#endif

    io_service_t service = IOServiceGetMatchingService(kIOMainPortDefault, IOServiceMatching("GoodbyeBigSlow"));
    if (!service) {
        fprintf(stderr, "Could not find GoodbyeBigSlow service\n");
        return 1;
    }

    io_connect_t connect;
    kern_return_t kr = IOServiceOpen(service, mach_task_self(), 0, &connect);
    IOObjectRelease(service);
    if (kr != KERN_SUCCESS) {
        fprintf(stderr, "Could not open service: %d\n", kr);
        return 1;
    }

    uint64_t scalarInput[4];
    uint32_t inputCount = 0;
    uint32_t selector = 0;
    uint64_t scalarOutput[1];
    uint32_t outputCount = 0;

    if (strcmp(argv[1], "plimit") == 0) {
        selector = kGBSMethodPLimit;
        scalarInput[0] = (argc > 2) ? strtoull(argv[2], NULL, 0) : 0;
        inputCount = 1;
    } else if (strcmp(argv[1], "gpu") == 0 && argc > 2 && strcmp(argv[2], "frequency") == 0) {
        selector = kGBSMethodGPUFrequency;
        scalarInput[0] = (argc > 3) ? strtoull(argv[3], NULL, 0) : 0;
        inputCount = 1;
    } else if (strcmp(argv[1], "cpu") == 0 && argc > 2) {
        if (strcmp(argv[2], "frequency") == 0) {
            selector = kGBSMethodCPUFrequency;
            scalarInput[0] = (argc > 3) ? strtoull(argv[3], NULL, 0) : 0;
            inputCount = 1;
        } else if (strcmp(argv[2], "idle") == 0) {
            selector = kGBSMethodCPUIdle;
            scalarInput[0] = (argc > 3) ? strtoull(argv[3], NULL, 0) : 0;
            inputCount = 1;
        } else if (strcmp(argv[2], "hwp") == 0) {
            selector = kGBSMethodCPUHWP;
            scalarInput[0] = (argc > 3) ? strtoull(argv[3], NULL, 0) : 0;
            inputCount = 1;
        } else {
            usage(argv[0]);
            IOServiceClose(connect);
            return 1;
        }
    } else if (strcmp(argv[1], "voltage") == 0) {
        selector = kGBSMethodVoltage;
        scalarInput[0] = (argc > 2) ? strtoull(argv[2], NULL, 0) : 0;
        scalarInput[1] = (argc > 3) ? strtoull(argv[3], NULL, 0) : 0;
        inputCount = 2;
    } else if (strncmp(argv[1], "msr", 3) == 0) {
        uint32_t mode = (strcmp(argv[1], "msr_mp") == 0) ? kGBSMSRModeAll : kGBSMSRModeSingle;
        if (argc == 2) {
            usage(argv[0]);
            IOServiceClose(connect);
            return 1;
        }
        uint32_t addr = (uint32_t)strtoul(argv[2], NULL, 0);
        if (argc == 3) {
            selector = kGBSMethodMSRRead;
            scalarInput[0] = addr;
            inputCount = 1;
            outputCount = 1;
        } else {
            selector = kGBSMethodMSRWrite;
            scalarInput[0] = addr;
            uint32_t op = kGBSMSROpWrite;
            uint64_t val = 0;
            if (argc == 4) {
                val = strtoull(argv[3], NULL, 0);
            } else {
                if (strcmp(argv[3], "and") == 0) op = kGBSMSROpAnd;
                else if (strcmp(argv[3], "or") == 0) op = kGBSMSROpOr;
                else if (strcmp(argv[3], "xor") == 0) op = kGBSMSROpXor;
                val = strtoull(argv[4], NULL, 0);
            }
            scalarInput[1] = val;
            scalarInput[2] = op;
            scalarInput[3] = mode;
            inputCount = 4;
        }
    } else {
        usage(argv[0]);
        IOServiceClose(connect);
        return 1;
    }

    kr = IOConnectCallExternalMethod(connect, selector, scalarInput, inputCount, NULL, 0, scalarOutput, &outputCount);
    if (kr == KERN_SUCCESS) {
        if (selector == kGBSMethodMSRRead) {
            printf("MSR[0x%lx] = 0x%lx\n", (unsigned long)scalarInput[0], (unsigned long)scalarOutput[0]);
        } else {
            printf("Success\n");
        }
    } else {
        fprintf(stderr, "Method call failed: 0x%x\n", kr);
    }

    IOServiceClose(connect);
    return 0;
}
