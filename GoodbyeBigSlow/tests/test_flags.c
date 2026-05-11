#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

// Mock implementations for functions used in GoodbyeBigSlow.c
#include "mach/mach_types.h"

void OSIncrementAtomic64(volatile int64_t *var) { (void)var; }
uint64_t rdmsr64(uint32_t msr) { (void)msr; return 0; }
void wrmsr64(uint32_t msr, uint64_t val) { (void)msr; (void)val; }
void IOLog(const char *format, ...) { (void)format; }
void IOSleep(uint32_t milliseconds) { (void)milliseconds; }
bool PE_parse_boot_argn(const char *arg_name, void *arg_ptr, int max_len) {
    (void)arg_name; (void)arg_ptr; (void)max_len;
    return false;
}
void mp_rendezvous_no_intrs(void (*func)(void *), void *arg) { (void)func; (void)arg; }
void cpuid(uint32_t *regs) { (void)regs; }

// Include source to test static functions
#define static
#include "../GoodbyeBigSlow.c"
#undef static

void test_eql_flag() {
    assert(eql_flag("turbo", "turbo", 5) == true);
    assert(eql_flag("turbo:abc", "turbo", 5) == true);
    assert(eql_flag("turb", "turbo", 5) == false);
    assert(eql_flag("turbx", "turbo", 5) == false);
    assert(eql_flag("turbo", "turbo", 4) == true);
    printf("test_eql_flag passed\n");
}

void test_has_flag() {
    const char *args = "-turbo:-speedstep:other";
    assert(has_flag(args, "-turbo") == true);
    assert(has_flag(args, "-speedstep") == true);
    assert(has_flag(args, "other") == false); // has_flag expects '-' or '+' at start of arg
    assert(has_flag(args, "-turb") == false);
    assert(has_flag(args, "-turboo") == false);

    const char *args2 = "-turbo";
    assert(has_flag(args2, "-turbo") == true);

    const char *args3 = "abc:-turbo:def";
    assert(has_flag(args3, "-turbo") == true);

    const char *args4 = "abc:-turbo";
    assert(has_flag(args4, "-turbo") == true);

    assert(has_flag("-turbo", "-turbo") == true);
    assert(has_flag("+turbo", "+turbo") == true);

    printf("test_has_flag passed\n");
}

int main() {
    test_eql_flag();
    test_has_flag();
    printf("All flag tests passed!\n");
    return 0;
}
