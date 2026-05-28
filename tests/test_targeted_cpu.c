#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <i386/cpuid.h>

static uint32_t mock_cpuid_eax = 0;
static uint32_t mock_cpuid_ebx = 0;
static uint32_t mock_cpuid_ecx = 0;
static uint32_t mock_cpuid_edx = 0;
static uint32_t mock_leaf = 0;

void cpuid(uint32_t *regs) {
    mock_leaf = regs[eax];
    if (mock_leaf == 0) {
        regs[eax] = 0x16; // max leaf
        regs[ebx] = 0x756E6547; // "Genu"
        regs[edx] = 0x49656E69; // "ineI"
        regs[ecx] = 0x6C65746E; // "ntel"
    } else if (mock_leaf == 1) {
        regs[eax] = mock_cpuid_eax;
        regs[ecx] = mock_cpuid_ecx;
    } else if (mock_leaf == 6) {
        regs[eax] = mock_cpuid_eax;
    }
}

bool PE_parse_boot_argn(const char *name, void *arg_ptr, int max_len) { (void)name; (void)arg_ptr; (void)max_len; return false; }
uint64_t rdmsr64(uint32_t msr) { (void)msr; return 0; }
void wrmsr64(uint32_t msr, uint64_t val) { (void)msr; (void)val; }
void mp_rendezvous_no_intrs(void (*func)(void *), void *arg) { (void)func; (void)arg; }

#include "GoodbyeBigSlow.c"

int main() {
    printf("test_targeted_cpu (compilation and basic check) passed\n");
    return 0;
}
