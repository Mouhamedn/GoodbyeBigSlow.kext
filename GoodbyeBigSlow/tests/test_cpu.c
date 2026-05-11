#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

// Mock implementations for functions used in GoodbyeBigSlow.c
#include "i386/cpuid.h"
#include "mach/mach_types.h"

// Define these before including the .c file so they don't conflict or to provide them if they are extern
void OSIncrementAtomic64(volatile int64_t *var) { (void)var; }
uint64_t rdmsr64(uint32_t msr) { (void)msr; return 0; }
void wrmsr64(uint32_t msr, uint64_t val) { (void)msr; (void)val; }
void IOLog(const char *format, ...) {
    (void)format;
}
void IOSleep(uint32_t milliseconds) { (void)milliseconds; }
bool PE_parse_boot_argn(const char *arg_name, void *arg_ptr, int max_len) {
    (void)arg_name; (void)arg_ptr; (void)max_len;
    return false;
}
void mp_rendezvous_no_intrs(void (*func)(void *), void *arg) { (void)func; (void)arg; }

// Include the source file to test static functions
#define static
#include "../GoodbyeBigSlow.c"
#undef static

// Mock CPUID logic
typedef struct {
    uint32_t input_eax;
    uint32_t output[4];
} cpuid_expectation;

static cpuid_expectation expectations[10];
static int num_expectations = 0;

void cpuid(uint32_t *regs) {
    uint32_t in_eax = regs[eax];
    for (int i = 0; i < num_expectations; i++) {
        if (expectations[i].input_eax == in_eax) {
            regs[eax] = expectations[i].output[eax];
            regs[ebx] = expectations[i].output[ebx];
            regs[ecx] = expectations[i].output[ecx];
            regs[edx] = expectations[i].output[edx];
            return;
        }
    }
    regs[eax] = 0;
    regs[ebx] = 0;
    regs[ecx] = 0;
    regs[edx] = 0;
}

void set_cpuid(uint32_t in_eax, uint32_t out_eax, uint32_t out_ebx, uint32_t out_ecx, uint32_t out_edx) {
    expectations[num_expectations].input_eax = in_eax;
    expectations[num_expectations].output[eax] = out_eax;
    expectations[num_expectations].output[ebx] = out_ebx;
    expectations[num_expectations].output[ecx] = out_ecx;
    expectations[num_expectations].output[edx] = out_edx;
    num_expectations++;
}

void reset_mocks() {
    num_expectations = 0;
}

void test_intel_cpu_ptm_supported() {
    reset_mocks();
    set_cpuid(0x00, 0x06, 0x756E6547, 0x6C65746E, 0x49656E69); // GenuineIntel, maxleaf 6
    set_cpuid(0x01, 0x000006E3, 0, 0, 0); // Family 6
    set_cpuid(0x06, 0x00000040, 0, 0, 0); // PTM supported (bit 6 of EAX)

    assert(using_targeted_intel_cpu() == true);
    printf("test_intel_cpu_ptm_supported passed\n");
}

void test_intel_cpu_ptm_not_supported() {
    reset_mocks();
    set_cpuid(0x00, 0x06, 0x756E6547, 0x6C65746E, 0x49656E69);
    set_cpuid(0x01, 0x000006E3, 0, 0, 0);
    set_cpuid(0x06, 0x00000000, 0, 0, 0);

    assert(using_targeted_intel_cpu() == false);
    printf("test_intel_cpu_ptm_not_supported passed\n");
}

void test_non_intel_cpu() {
    reset_mocks();
    set_cpuid(0x00, 0x06, 0x41757468, 0x444d4163, 0x656e7469); // AuthenticAMD

    assert(using_targeted_intel_cpu() == false);
    printf("test_non_intel_cpu passed\n");
}

void test_intel_cpu_low_maxval() {
    reset_mocks();
    set_cpuid(0x00, 0x00, 0x756E6547, 0x6C65746E, 0x49656E69); // maxval = 0

    assert(using_targeted_intel_cpu() == false);
    printf("test_intel_cpu_low_maxval passed\n");
}

void test_intel_cpu_no_leaf_6() {
    reset_mocks();
    set_cpuid(0x00, 0x05, 0x756E6547, 0x6C65746E, 0x49656E69); // maxleaf 5
    set_cpuid(0x01, 0x000006E3, 0, 0, 0);

    assert(using_targeted_intel_cpu() == false);
    printf("test_intel_cpu_no_leaf_6 passed\n");
}

void test_intel_wrong_family() {
    reset_mocks();
    set_cpuid(0x00, 0x06, 0x756E6547, 0x6C65746E, 0x49656E69);
    set_cpuid(0x01, 0x00000FE3, 0, 0, 0); // Family 0xF

    assert(using_targeted_intel_cpu() == false);
    printf("test_intel_wrong_family passed\n");
}

int main() {
    test_intel_cpu_ptm_supported();
    test_intel_cpu_ptm_not_supported();
    test_non_intel_cpu();
    test_intel_cpu_low_maxval();
    test_intel_cpu_no_leaf_6();
    test_intel_wrong_family();
    printf("All using_targeted_intel_cpu tests passed!\n");
    return 0;
}
