#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>

#include "i386/cpuid.h"

// Mock state
static uint32_t mock_eax[10];
static uint32_t mock_ebx[10];
static uint32_t mock_ecx[10];
static uint32_t mock_edx[10];

void cpuid(uint32_t *regs) {
    uint32_t leaf = regs[eax];
    if (leaf < 10) {
        regs[eax] = mock_eax[leaf];
        regs[ebx] = mock_ebx[leaf];
        regs[ecx] = mock_ecx[leaf];
        regs[edx] = mock_edx[leaf];
    }
}

// Stub for PE_parse_boot_argn which is used in GoodbyeBigSlow.c but not needed for this test
typedef int boolean_t;
boolean_t PE_parse_boot_argn(const char *arg_string, void *arg_ptr, int max_len) {
    return 0;
}

// Include the source file to test static functions
#include "../GoodbyeBigSlow/GoodbyeBigSlow.c"

void reset_mocks() {
    memset(mock_eax, 0, sizeof(mock_eax));
    memset(mock_ebx, 0, sizeof(mock_ebx));
    memset(mock_ecx, 0, sizeof(mock_ecx));
    memset(mock_edx, 0, sizeof(mock_edx));
}

void test_genuine_intel_supported() {
    reset_mocks();
    // Leaf 0: Max Leaf and Vendor String
    mock_eax[0] = 6;
    mock_ebx[0] = 0x756E6547; // Genu
    mock_edx[0] = 0x49656E69; // ineI
    mock_ecx[0] = 0x6C65746E; // ntel

    // Leaf 1: Family/Model/Stepping
    mock_eax[1] = (6 << 8); // Family 6

    // Leaf 6: Thermal and Power Management Features
    mock_eax[6] = (1 << 6); // PTM supported

    assert(using_targeted_intel_cpu() == true);
    printf("test_genuine_intel_supported passed\n");
}

void test_non_intel_cpu() {
    reset_mocks();
    // Leaf 0: Max Leaf and Vendor String (AuthenticAMD)
    mock_eax[0] = 6;
    mock_ebx[0] = 0x68747541;
    mock_edx[0] = 0x69746E65;
    mock_ecx[0] = 0x444D4163;

    assert(using_targeted_intel_cpu() == false);
    printf("test_non_intel_cpu passed\n");
}

void test_wrong_family() {
    reset_mocks();
    // Leaf 0: Max Leaf and Vendor String
    mock_eax[0] = 6;
    mock_ebx[0] = 0x756E6547;
    mock_edx[0] = 0x49656E69;
    mock_ecx[0] = 0x6C65746E;

    // Leaf 1: Family 5
    mock_eax[1] = (5 << 8);

    assert(using_targeted_intel_cpu() == false);
    printf("test_wrong_family passed\n");
}

void test_low_max_leaf() {
    reset_mocks();
    // Leaf 0: Max Leaf 5
    mock_eax[0] = 5;
    mock_ebx[0] = 0x756E6547;
    mock_edx[0] = 0x49656E69;
    mock_ecx[0] = 0x6C65746E;

    // Leaf 1: Family 6
    mock_eax[1] = (6 << 8);

    assert(using_targeted_intel_cpu() == false);
    printf("test_low_max_leaf passed\n");
}

void test_no_ptm_support() {
    reset_mocks();
    // Leaf 0: Max Leaf 6
    mock_eax[0] = 6;
    mock_ebx[0] = 0x756E6547;
    mock_edx[0] = 0x49656E69;
    mock_ecx[0] = 0x6C65746E;

    // Leaf 1: Family 6
    mock_eax[1] = (6 << 8);

    // Leaf 6: PTM NOT supported (bit 6 is 0)
    mock_eax[6] = 0;

    assert(using_targeted_intel_cpu() == false);
    printf("test_no_ptm_support passed\n");
}

int main() {
    test_genuine_intel_supported();
    test_non_intel_cpu();
    test_wrong_family();
    test_low_max_leaf();
    test_no_ptm_support();
    printf("All tests passed!\n");
    return 0;
}
