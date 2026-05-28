#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>

char *PE_boot_args(void) { return "GoodbyeBigSlow=-turbo:-speedstep"; }
uint64_t rdmsr64(uint32_t msr) { (void)msr; return 0; }
void wrmsr64(uint32_t msr, uint64_t val) { (void)msr; (void)val; }
void cpuid(uint32_t *regs) { (void)regs; }
void mp_rendezvous_no_intrs(void (*func)(void *), void *arg) { (void)func; (void)arg; }

#include "GoodbyeBigSlow.c"

int main() {
    // Current has_boot_arg_flag only checks for exact matches of flags.
    // "GoodbyeBigSlow=-turbo:-speedstep" is one single boot arg in darwin.
    // My has_boot_arg_flag scans the whole string for ":flag" or " flag" or "flag"

    // Wait, PE_boot_args() returns the whole boot-args string.
    // If boot-args is "GoodbyeBigSlow=-turbo:-speedstep some-other-arg"
    // has_boot_arg_flag("-turbo") should find it if it handles ':'

    assert(has_boot_arg_flag("GoodbyeBigSlow=-turbo:-speedstep") == true);
    assert(has_boot_arg_flag("-turbo") == true);
    assert(has_boot_arg_flag("-speedstep") == true);
    printf("test_parser passed\n");
    return 0;
}
