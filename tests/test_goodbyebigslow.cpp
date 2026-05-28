#include <assert.h>
#include <string.h>
#include <cstdio>
#include <map>
#include "i386/cpuid.h"
#include "i386/proc_reg.h"
#include "GoodbyeBigSlow.hpp"

// Global mock state
std::map<uint32_t, cpuid_result> mock_cpuid_results;
std::map<uint32_t, uint64_t> mock_msrs;
const char* mock_boot_args = "";

// Mock external functions
extern "C" {
    void mp_rendezvous_no_intrs(void (*func)(void *), void *arg) {
        func(arg);
    }

    bool PE_parse_boot_argn(const char *key, void *arg, int max_len) {
        if (strcmp(key, "GoodbyeBigSlow") == 0 && strlen(mock_boot_args) > 0) {
            strncpy((char*)arg, mock_boot_args, max_len);
            return true;
        }
        return false;
    }
}

void setup_supported_intel_cpu() {
    mock_cpuid_results.clear();
    mock_msrs.clear();

    // leaf 0: GenuineIntel
    mock_cpuid_results[0x00] = {{0x06, 0x756E6547, 0x6C65746E, 0x49656E69}};
    // leaf 1: Family 6
    mock_cpuid_results[0x01] = {{0x00000600, 0, 1 << 7, 0}};
    // leaf 6: PTM support (bit 6), Turbo Boost (bit 1)
    mock_cpuid_results[0x06] = {{1 << 6 | 1 << 1 | 1 << 4, 0, 0, 0}};

    // MSRs
    mock_msrs[0x1FC] = 1; // MSR_IA32_POWER_CTL: PROCHOT enabled
    mock_msrs[0x1A0] = 1ULL << 16; // MSR_IA32_MISC_ENABLE: SpeedStep enabled (bit 16)
    mock_msrs[0x1B1] = 0; // MSR_IA32_PACKAGE_THERM_STATUS
    mock_msrs[0x19C] = 0; // MSR_IA32_THERM_STATUS
}

void test_basic_lifecycle() {
    printf("Running test_basic_lifecycle...\n");
    setup_supported_intel_cpu();
    mock_boot_args = "";

    GoodbyeBigSlow *driver = new GoodbyeBigSlow();
    assert(driver->init(NULL) == true);

    SInt32 score = 0;
    assert(driver->probe(NULL, &score) != NULL);

    assert(driver->start(NULL) == true);

    // Verify PROCHOT was disabled
    assert(!(mock_msrs[0x1FC] & 1));

    driver->stop(NULL);
    driver->free();
    delete driver;
    printf("test_basic_lifecycle passed!\n");
}

void test_boot_args() {
    printf("Running test_boot_args...\n");
    setup_supported_intel_cpu();
    mock_boot_args = "-turbo:-speedstep";

    GoodbyeBigSlow *driver = new GoodbyeBigSlow();
    assert(driver->init(NULL) == true);
    assert(driver->start(NULL) == true);

    // Verify Turbo Boost disabled (bit 38)
    assert(mock_msrs[0x1A0] & (1ULL << 38));
    // Verify SpeedStep disabled (bit 16)
    assert(!(mock_msrs[0x1A0] & (1ULL << 16)));

    driver->stop(NULL);
    driver->free();
    delete driver;
    printf("test_boot_args passed!\n");
}

void test_unsupported_cpu() {
    printf("Running test_unsupported_cpu...\n");
    mock_cpuid_results.clear();
    // leaf 0: AuthenticAMD
    mock_cpuid_results[0x00] = {{0x06, 0x68747541, 0x444d4163, 0x69746e65}};

    GoodbyeBigSlow *driver = new GoodbyeBigSlow();
    assert(driver->init(NULL) == true);
    assert(driver->start(NULL) == false); // Should fail due to unsupported CPU

    driver->free();
    delete driver;
    printf("test_unsupported_cpu passed!\n");
}

int main() {
    test_basic_lifecycle();
    test_boot_args();
    test_unsupported_cpu();
    printf("All tests passed!\n");
    return 0;
}
