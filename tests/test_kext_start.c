#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

// Mock kernel headers
#include "libkern/libkern.h"
#include "mach/mach_types.h"
#include "i386/proc_reg.h"
#include "i386/cpuid.h"
#include "sys/ioccom.h"
#include "IOKit/IOLib.h"

// Globals to store mock states
static char mock_boot_args[256];
static bool mock_boot_arg_exists = false;
static uint64_t mock_msr_values[0x1000];
static int mp_rendezvous_called = 0;

// Mock implementations
void cpuid(uint32_t *regs) {
    uint32_t leaf = regs[eax];
    if (leaf == 0) {
        regs[eax] = 0x06;
        regs[ebx] = 0x756E6547; // Genu
        regs[edx] = 0x49656E69; // ineI
        regs[ecx] = 0x6C65746E; // ntel
    } else if (leaf == 1) {
        regs[eax] = (0x06 << 8); // Family 6
        regs[ecx] = (1 << 7);    // SpeedStep support (bit 7)
    } else if (leaf == 6) {
        regs[eax] = (1 << 6) | (1 << 1); // PTM support (bit 6) and Turbo support (bit 1)
    } else {
        memset(regs, 0, 4 * sizeof(uint32_t));
    }
}

// Special cpuid for unsupported CPU
static bool force_unsupported_cpu = false;
void cpuid_mock(uint32_t *regs) {
    if (force_unsupported_cpu) {
        memset(regs, 0, 4 * sizeof(uint32_t));
        return;
    }
    cpuid(regs);
}
#define cpuid cpuid_mock

uint64_t rdmsr64(uint32_t msr) {
    if (msr < 0x1000) return mock_msr_values[msr];
    return 0;
}

void wrmsr64(uint32_t msr, uint64_t val) {
    if (msr < 0x1000) mock_msr_values[msr] = val;
}

void mp_rendezvous_no_intrs(void (*func)(void *), void *arg) {
    mp_rendezvous_called++;
    // Simulate multi-core rendezvous by calling it once
    func(arg);
}

int PE_parse_boot_argn(const char *key, void *arg_ptr, int max_len) {
    if (strcmp(key, "GoodbyeBigSlow") == 0 && mock_boot_arg_exists) {
        strncpy((char *)arg_ptr, mock_boot_args, max_len);
        return 1;
    }
    return 0;
}

// Include the implementation
#include "../GoodbyeBigSlow/GoodbyeBigSlow.c"

void reset_mocks() {
    memset(mock_msr_values, 0, sizeof(mock_msr_values));
    memset(mock_boot_args, 0, sizeof(mock_boot_args));
    mock_boot_arg_exists = false;
    mp_rendezvous_called = 0;
    force_unsupported_cpu = false;
}

void test_unsupported_cpu() {
    reset_mocks();
    printf("Running test_unsupported_cpu...\n");
    force_unsupported_cpu = true;
    kern_return_t res = kext_start(NULL, NULL);
    assert(res == KERN_FAILURE);
    printf("test_unsupported_cpu passed!\n");
}

void test_supported_cpu_no_args() {
    reset_mocks();
    printf("Running test_supported_cpu_no_args...\n");

    mock_msr_values[MSR_IA32_POWER_CTL] = kMsrEnableProcHot;

    kern_return_t res = kext_start(NULL, NULL);
    assert(res == KERN_SUCCESS);

    // Should still deassert prochot
    assert(!(mock_msr_values[MSR_IA32_POWER_CTL] & kMsrEnableProcHot));
    assert(mp_rendezvous_called > 0);
    printf("test_supported_cpu_no_args passed!\n");
}

void test_supported_cpu_turbo_arg() {
    reset_mocks();
    printf("Running test_supported_cpu_turbo_arg...\n");

    mock_boot_arg_exists = true;
    strcpy(mock_boot_args, "-turbo");
    mock_msr_values[MSR_IA32_POWER_CTL] = kMsrEnableProcHot;
    mock_msr_values[MSR_IA32_MISC_ENABLE] = 0;

    kern_return_t res = kext_start(NULL, NULL);
    assert(res == KERN_SUCCESS);

    // Verify turbo disabled
    assert(mock_msr_values[MSR_IA32_MISC_ENABLE] & kMsrDisableTurboBoost);
    // Verify prochot deasserted
    assert(!(mock_msr_values[MSR_IA32_POWER_CTL] & kMsrEnableProcHot));

    printf("test_supported_cpu_turbo_arg passed!\n");
}

void test_supported_cpu_speedstep_arg() {
    reset_mocks();
    printf("Running test_supported_cpu_speedstep_arg...\n");

    mock_boot_arg_exists = true;
    strcpy(mock_boot_args, "-speedstep");
    mock_msr_values[MSR_IA32_POWER_CTL] = kMsrEnableProcHot;
    mock_msr_values[MSR_IA32_MISC_ENABLE] = kMsrEnableSpeedStep;

    kern_return_t res = kext_start(NULL, NULL);
    assert(res == KERN_SUCCESS);

    // Verify speedstep disabled
    assert(!(mock_msr_values[MSR_IA32_MISC_ENABLE] & kMsrEnableSpeedStep));
    // Verify prochot deasserted
    assert(!(mock_msr_values[MSR_IA32_POWER_CTL] & kMsrEnableProcHot));

    printf("test_supported_cpu_speedstep_arg passed!\n");
}

void test_supported_cpu_both_args() {
    reset_mocks();
    printf("Running test_supported_cpu_both_args...\n");

    mock_boot_arg_exists = true;
    strcpy(mock_boot_args, "-turbo:-speedstep");
    mock_msr_values[MSR_IA32_POWER_CTL] = kMsrEnableProcHot;
    mock_msr_values[MSR_IA32_MISC_ENABLE] = kMsrEnableSpeedStep;

    kern_return_t res = kext_start(NULL, NULL);
    assert(res == KERN_SUCCESS);

    // Verify both disabled
    assert(mock_msr_values[MSR_IA32_MISC_ENABLE] & kMsrDisableTurboBoost);
    assert(!(mock_msr_values[MSR_IA32_MISC_ENABLE] & kMsrEnableSpeedStep));
    // Verify prochot deasserted
    assert(!(mock_msr_values[MSR_IA32_POWER_CTL] & kMsrEnableProcHot));

    printf("test_supported_cpu_both_args passed!\n");
}

int main() {
    test_unsupported_cpu();
    test_supported_cpu_no_args();
    test_supported_cpu_turbo_arg();
    test_supported_cpu_speedstep_arg();
    test_supported_cpu_both_args();

    printf("\nAll kext_start tests passed!\n");
    return 0;
}
