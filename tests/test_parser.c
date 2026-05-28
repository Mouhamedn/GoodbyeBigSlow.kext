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

uint64_t rdmsr64(uint32_t msr) { (void)msr; return 0; }
void wrmsr64(uint32_t msr, uint64_t val) { (void)msr; (void)val; }
void cpuid(uint32_t *regs) { (void)regs; }
void mp_rendezvous_no_intrs(void (*func)(void *), void *arg) { (void)func; (void)arg; }

#include "GoodbyeBigSlow.c"

int main() {
    strcpy(mock_boot_arg_val, "-turbo:-speedstep");

    char boot_args[BOOT_ARGS_SIZE];
    assert(PE_parse_boot_argn("GoodbyeBigSlow", boot_args, BOOT_ARGS_SIZE));

    assert(has_flag(boot_args, "-turbo") == true);
    assert(has_flag(boot_args, "-speedstep") == true);
    assert(has_flag(boot_args, "-missing") == false);

    printf("test_parser passed\n");
    return 0;
}
