#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>

// Mock data
static char mock_boot_args[256];
char *PE_boot_args(void) { return mock_boot_args; }

// Dummy implementations for things needed by GoodbyeBigSlow.c
uint64_t rdmsr64(uint32_t msr) { (void)msr; return 0; }
void wrmsr64(uint32_t msr, uint64_t val) { (void)msr; (void)val; }
void cpuid(uint32_t *regs) { (void)regs; }
void mp_rendezvous_no_intrs(void (*func)(void *), void *arg) { (void)func; (void)arg; }

#include "GoodbyeBigSlow.c"

void test_has_boot_arg_flag() {
    strcpy(mock_boot_args, "-turbo:-speedstep some-other-arg");
    assert(has_boot_arg_flag("-turbo") == true);
    assert(has_boot_arg_flag("-speedstep") == true);
    assert(has_boot_arg_flag("some-other-arg") == true);
    assert(has_boot_arg_flag("-missing") == false);

    strcpy(mock_boot_args, "flag1 flag2:flag3");
    assert(has_boot_arg_flag("flag1") == true);
    assert(has_boot_arg_flag("flag2") == true);
    assert(has_boot_arg_flag("flag3") == true);

    printf("test_has_boot_arg_flag passed\n");
}

int main() {
    test_has_boot_arg_flag();
    return 0;
}
