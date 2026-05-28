#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "i386/cpuid.h"

// Mock state
typedef struct {
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
} cpuid_result_t;

#define MAX_CPUID_LEAF 256
static cpuid_result_t mock_cpuid_results[MAX_CPUID_LEAF];

void set_mock_cpuid(uint32_t leaf, uint32_t eax_val, uint32_t ebx_val, uint32_t ecx_val, uint32_t edx_val) {
    if (leaf < MAX_CPUID_LEAF) {
        mock_cpuid_results[leaf].eax = eax_val;
        mock_cpuid_results[leaf].ebx = ebx_val;
        mock_cpuid_results[leaf].ecx = ecx_val;
        mock_cpuid_results[leaf].edx = edx_val;
    }
}

// Mock implementation of cpuid
extern "C" void cpuid(uint32_t *registers) {
    uint32_t leaf = registers[eax];
    if (leaf < MAX_CPUID_LEAF) {
        registers[eax] = mock_cpuid_results[leaf].eax;
        registers[ebx] = mock_cpuid_results[leaf].ebx;
        registers[ecx] = mock_cpuid_results[leaf].ecx;
        registers[edx] = mock_cpuid_results[leaf].edx;
    } else {
        registers[eax] = 0;
        registers[ebx] = 0;
        registers[ecx] = 0;
        registers[edx] = 0;
    }
}

// Stubs for other kernel functions
extern "C" uint64_t rdmsr64(uint32_t msr) { return 0; }
extern "C" void wrmsr64(uint32_t msr, uint64_t val) {}
extern "C" void IOLog(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}
extern "C" void IOSleep(uint32_t milliseconds) {}
extern "C" void OSIncrementAtomic64(volatile int64_t *dst) { (*dst)++; }
extern "C" {
    void mp_rendezvous_no_intrs(void (*func)(void *), void *arg) {}
    bool PE_parse_boot_argn(const char *argName, void *argVal, int maxLen) { return false; }
}

// Include the source file to test the static function
#include "../GoodbyeBigSlow/GoodbyeBigSlow.c"

// Test helpers
#define ASSERT_TRUE(cond) do { \
    if (!(cond)) { \
        printf("FAIL: %s at line %d\n", #cond, __LINE__); \
        exit(1); \
    } \
} while(0)

#define ASSERT_FALSE(cond) do { \
    if ((cond)) { \
        printf("FAIL: %s at line %d\n", #cond, __LINE__); \
        exit(1); \
    } \
} while(0)

void test_non_intel_cpu() {
    printf("Running test_non_intel_cpu...\n");
    memset(mock_cpuid_results, 0, sizeof(mock_cpuid_results));
    // Set vendor to "AuthenticAMD"
    set_mock_cpuid(0, 0x0D, 0x68747541, 0x444d4163, 0x69746e65);
    ASSERT_FALSE(using_targeted_intel_cpu());
}

void test_intel_wrong_family() {
    printf("Running test_intel_wrong_family...\n");
    memset(mock_cpuid_results, 0, sizeof(mock_cpuid_results));
    // GenuineIntel, max leaf 1
    set_mock_cpuid(0, 1, 0x756E6547, 0x6C65746E, 0x49656E69);
    // Family 15 (NetBurst)
    set_mock_cpuid(1, (15 << 8), 0, 0, 0);
    ASSERT_FALSE(using_targeted_intel_cpu());
}

void test_intel_family_6_no_ptm_leaf() {
    printf("Running test_intel_family_6_no_ptm_leaf...\n");
    memset(mock_cpuid_results, 0, sizeof(mock_cpuid_results));
    // GenuineIntel, max leaf 5 (less than 6)
    set_mock_cpuid(0, 5, 0x756E6547, 0x6C65746E, 0x49656E69);
    // Family 6
    set_mock_cpuid(1, (6 << 8), 0, 0, 0);
    ASSERT_FALSE(using_targeted_intel_cpu());
}

void test_intel_family_6_ptm_bit_not_set() {
    printf("Running test_intel_family_6_ptm_bit_not_set...\n");
    memset(mock_cpuid_results, 0, sizeof(mock_cpuid_results));
    // GenuineIntel, max leaf 6
    set_mock_cpuid(0, 6, 0x756E6547, 0x6C65746E, 0x49656E69);
    // Family 6
    set_mock_cpuid(1, (6 << 8), 0, 0, 0);
    // Leaf 6, EAX bit 6 (PTM) is NOT set
    set_mock_cpuid(6, 0, 0, 0, 0);
    ASSERT_FALSE(using_targeted_intel_cpu());
}

void test_intel_family_6_ptm_bit_set() {
    printf("Running test_intel_family_6_ptm_bit_set...\n");
    memset(mock_cpuid_results, 0, sizeof(mock_cpuid_results));
    // GenuineIntel, max leaf 6
    set_mock_cpuid(0, 6, 0x756E6547, 0x6C65746E, 0x49656E69);
    // Family 6
    set_mock_cpuid(1, (6 << 8), 0, 0, 0);
    // Leaf 6, EAX bit 6 (PTM) IS set
    set_mock_cpuid(6, (1 << 6), 0, 0, 0);
    ASSERT_TRUE(using_targeted_intel_cpu());
}

int main() {
    test_non_intel_cpu();
    test_intel_wrong_family();
    test_intel_family_6_no_ptm_leaf();
    test_intel_family_6_ptm_bit_not_set();
    test_intel_family_6_ptm_bit_set();
    printf("All tests passed!\n");
    return 0;
}
