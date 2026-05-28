#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>

static char mock_boot_arg_val[256];
bool PE_parse_boot_argn(const char *name, void *arg_ptr, int max_len) {
    if (strcmp(name, "GoodbyeBigSlow") == 0) {
        strncpy((char *)arg_ptr, mock_boot_arg_val, max_len);
        return true;
    }
    return false;
}

// Dummy implementations for things needed by GoodbyeBigSlow.c
uint64_t rdmsr64(uint32_t msr) { (void)msr; return 0; }
void wrmsr64(uint32_t msr, uint64_t val) { (void)msr; (void)val; }
void cpuid(uint32_t *regs) { (void)regs; }
void mp_rendezvous_no_intrs(void (*func)(void *), void *arg) { (void)func; (void)arg; }

#include "GoodbyeBigSlow.c"

void test_has_flag_logic() {
    assert(has_flag("-turbo:-speedstep", "-turbo") == true);
    assert(has_flag("-turbo:-speedstep", "-speedstep") == true);
    assert(has_flag("-turbo:-speedstep", "-missing") == false);

    assert(has_flag("flag1:flag2", "flag1") == false); // only +/- flags
    assert(has_flag("-flag1:+flag2", "-flag1") == true);
    assert(has_flag("-flag1:+flag2", "+flag2") == true);

    printf("test_has_flag passed\n");
}

int main() {
    test_has_flag_logic();
    return 0;
}
