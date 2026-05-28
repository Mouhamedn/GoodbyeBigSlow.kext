#include <libkern/libkern.h>
#include <mach/mach_types.h>
#include <i386/proc_reg.h>
#include <i386/cpuid.h>
#include <sys/ioccom.h>
#include <IOKit/IOLib.h>

// IDEA: kick X86PlatformShim.kext off the candidate list (failed)
// IDEA: patch __ZN15X86PlatformShim18sendHardPLimitXCPMEi (Lilu.kext)
// IDEA: SMC_Write(KPPW, KernelProtectionPassword) SMC_Write(MSAL, 0bX0XX1100)
// TODO: GoodbyeBigSlowClient
//       $ plimit [<frequency>]
//       $ gpu frequency [<frequency>]
//       $ cpu frequency [<frequency>]
//       $ cpu idle [<percent>]
//       198H MSR_IA32_PERF_STS (IA32_PERF_STATUS; CPUID.01H:ECX[7]=1)
//       199H MSR_IA32_PERF_CTL (mask: 0xFF00 for targeted CPUs)
//       $ cpu hwp [<frequency>]
//       770H IA32_PM_ENABLE[0] (alternative; Write-Once; CPUID.06H:EAX.[7]=1)
//       $ voltage <min> <max>
//       $ msr[_mp|_mt] <address> [[and|or|xor] <0xHHHHHHHHHHHHHHHH>]

#ifdef __cplusplus
extern "C" {
#endif

// https://developer.apple.com/documentation/kernel/1576460-osincrementatomic
// https://lwn.net/Articles/793253/
#define VOLATILE_ACCESS(x) (*((volatile __typeof__(x) *)&(x)))

#ifndef MSR_IA32_PLATFORM_ID
#define MSR_IA32_PLATFORM_ID 0x17
#endif
#ifndef MSR_IA32_PERF_STS
#define MSR_IA32_PERF_STS 0x198
#endif
#ifndef MSR_IA32_PERF_CTL
#define MSR_IA32_PERF_CTL 0x199
#endif
#ifndef MSR_IA32_THERM_STATUS
#define MSR_IA32_THERM_STATUS 0x19C
#endif
#ifndef MSR_IA32_MISC_ENABLE
#define MSR_IA32_MISC_ENABLE 0x1A0
#endif
#ifndef MSR_IA32_PACKAGE_THERM_STATUS
#define MSR_IA32_PACKAGE_THERM_STATUS 0x1B1
#endif
#ifndef MSR_IA32_POWER_CTL
#define MSR_IA32_POWER_CTL 0x1FC
#endif

// Credit to https://www.techpowerup.com/download/techpowerup-throttlestop/
const uint64_t kMsrEnableProcHot = 1;

const uint64_t kMsrEnableSpeedStep = 1ULL << 16;
const uint64_t kMsrSpeedStepLock = 1ULL << 20;
const uint64_t kMsrDisableTurboBoost = 1ULL << 38;

const uint64_t kMsrThermalStatusMask = 0x28A;  // 0b1010001010

// https://github.com/apple/darwin-xnu/blob/main/osfmk/i386/mp.h
// Perform actions on all processor cores.
extern void mp_rendezvous_no_intrs(void (*func)(void *), void *arg);

static bool is_msr_allowed(uint32_t msr)
{
    switch (msr) {
        case MSR_IA32_PLATFORM_ID:
        case MSR_IA32_PERF_STS:
        case MSR_IA32_PERF_CTL:
        case MSR_IA32_THERM_STATUS:
        case MSR_IA32_MISC_ENABLE:
        case MSR_IA32_PACKAGE_THERM_STATUS:
        case MSR_IA32_POWER_CTL:
            return true;
        default:
            return false;
    }
}

struct mp_msr_args {
    uint32_t msr;
    uint64_t value;
};

static void mp_wrmsr(void *data)
{
    struct mp_msr_args *args = (struct mp_msr_args *)data;
    wrmsr64(args->msr, args->value);
}

static void wrmsr_all(uint32_t msr, uint64_t value)
{
    struct mp_msr_args args = {msr, value};
    mp_rendezvous_no_intrs(mp_wrmsr, &args);
}

// ALERT: Toggling PROCHOT more than once in ~2 ms period can result in
//        constant Pn state (Low Frequency Mode) of the processor.
static void mp_deassert_prochot(void *data)
{
    SInt64 *counts = (SInt64 *)data;
    uint64_t old_bits = rdmsr64(MSR_IA32_POWER_CTL);
    uint64_t new_bits = old_bits & ~kMsrEnableProcHot;

    // hopefully not to cause invalid write and the black screen of death
    if (old_bits != new_bits) {
        wrmsr64(MSR_IA32_POWER_CTL, new_bits);
        IODelay(1000);
        if (!(rdmsr64(MSR_IA32_POWER_CTL) & kMsrEnableProcHot)) {
            OSIncrementAtomic64(&counts[1]);
        }
    } else {
        OSIncrementAtomic64(&counts[1]);
    }
    OSIncrementAtomic64(&counts[0]);
}

static void mp_log_prochot(void *data)
{
    uint64_t mask = *((uint64_t *)data);
    uint64_t old_bits = rdmsr64(MSR_IA32_THERM_STATUS);
    uint64_t new_bits = old_bits & ~mask;

    if (old_bits != new_bits) {
        wrmsr64(MSR_IA32_THERM_STATUS, new_bits);
    }
}

static void log_prochot(void)
{
    uint64_t mask = kMsrThermalStatusMask;
    uint64_t old_bits = rdmsr64(MSR_IA32_PACKAGE_THERM_STATUS);
    uint64_t new_bits = old_bits & ~mask;

    if (old_bits & (1 << 11)) {
        uint32_t registers[4] = {[eax]=6, [ebx]=0, [ecx]=0, [edx]=0};
        cpuid(registers);

        if (registers[eax] & (1 << 4)) {
            mask |= (1 << 11);
            new_bits &= ~mask;
        }
    }
    if (old_bits != new_bits) {
        wrmsr64(MSR_IA32_PACKAGE_THERM_STATUS, new_bits);
    }
    mp_rendezvous_no_intrs(mp_log_prochot, &mask);
}

static bool deassert_prochot(void)
{
    SInt64 counts[2] __attribute__((aligned(sizeof(SInt64)))) = {0, 0};

    mp_rendezvous_no_intrs(mp_deassert_prochot, counts);

    if (counts[0] == counts[1]) {
        log_prochot();
        return true;
    }
    return false;
}

static bool disable_turbo(void)
{
    uint32_t registers[4] = {[eax]=6, [ebx]=0, [ecx]=0, [edx]=0};
    cpuid(registers);

    if (registers[eax] & (1 << 1)) {
        uint64_t old_bits = rdmsr64(MSR_IA32_MISC_ENABLE);
        uint64_t new_bits = old_bits | kMsrDisableTurboBoost;

        if (old_bits != new_bits) {
            wrmsr_all(MSR_IA32_MISC_ENABLE, new_bits);
            IODelay(1000);
            return rdmsr64(MSR_IA32_MISC_ENABLE) & kMsrDisableTurboBoost;
        }
    }
    return true;
}

static bool disable_speedstep(void)
{
    uint32_t registers[4] = {[eax]=1, [ebx]=0, [ecx]=0, [edx]=0};
    cpuid(registers);

    if (registers[ecx] & (1 << 7)) {
        uint64_t old_bits = rdmsr64(MSR_IA32_MISC_ENABLE);

        if (old_bits & kMsrSpeedStepLock) {
            return false;
        }

        uint64_t new_bits = old_bits & ~kMsrEnableSpeedStep;

        if (old_bits != new_bits) {
            wrmsr_all(MSR_IA32_MISC_ENABLE, new_bits);
            IODelay(1000);
            return !(rdmsr64(MSR_IA32_MISC_ENABLE) & kMsrEnableSpeedStep);
        }
    }
    return true;
}

static uint8_t get_display_family(void)
{
    uint32_t registers[4] = {[eax]=1, [ebx]=0, [ecx]=0, [edx]=0};
    cpuid(registers);
    uint8_t family = (registers[eax] >> 8) & 0x0F;
    if (family == 0x0F) {
        family += (registers[eax] >> 20) & 0xFF;
    }
    return family;
}

static uint8_t get_display_model(void)
{
    uint32_t registers[4] = {[eax]=1, [ebx]=0, [ecx]=0, [edx]=0};
    cpuid(registers);
    uint8_t family = (registers[eax] >> 8) & 0x0F;
    uint8_t model = (registers[eax] >> 4) & 0x0F;
    if (family == 0x06 || family == 0x0F) {
        model += ((registers[eax] >> 16) & 0x0F) << 4;
    }
    return model;
}

static bool is_xeon(void)
{
    char brand[49];
    uint32_t registers[4];

    registers[eax] = 0x80000000;
    cpuid(registers);
    if (registers[eax] < 0x80000004) return false;

    for (uint32_t i = 0; i < 3; i++) {
        registers[eax] = 0x80000002 + i;
        cpuid(registers);
        memcpy(brand + i * 16, registers, 16);
    }
    brand[48] = '\0';

    for (int i = 0; i <= (int)sizeof(brand) - 5; i++) {
        if ((brand[i] == 'x' || brand[i] == 'X') &&
            (brand[i+1] == 'e' || brand[i+1] == 'E') &&
            (brand[i+2] == 'o' || brand[i+2] == 'O') &&
            (brand[i+3] == 'n' || brand[i+3] == 'N')) {
            return true;
        }
    }
    return false;
}

static bool is_model_supported(uint8_t model)
{
    switch (model) {
        case 0x1A: case 0x1E: case 0x1F: case 0x25: case 0x2A: case 0x2C:
        case 0x2D: case 0x2E: case 0x2F: case 0x3A: case 0x3C: case 0x3D:
        case 0x3E: case 0x3F: case 0x45: case 0x46: case 0x47: case 0x4E:
        case 0x4F: case 0x55: case 0x56: case 0x5C: case 0x5E: case 0x66:
        case 0x6A: case 0x6C: case 0x7A: case 0x7D: case 0x7E: case 0x86:
        case 0x8C: case 0x8D: case 0x8E: case 0x96: case 0x97: case 0x9A:
        case 0x9C: case 0x9E: case 0xA5: case 0xA6: case 0xB7: case 0xBA:
        case 0xBF:
            return true;
        default:
            return false;
    }
}

static bool using_targeted_intel_cpu(void)
{
    uint32_t registers[4] = {[eax]=0x00, [ebx]=0xFF, [ecx]=0xFF, [edx]=0xFF};
    cpuid(registers);
    uint32_t maxval = registers[eax];

    bool GenuineIntel = registers[ebx] == 0x756E6547 &&
                        registers[edx] == 0x49656E69 &&
                        registers[ecx] == 0x6C65746E &&
                        maxval >= 0x01;

    if (GenuineIntel) {
        if (get_display_family() == 0x06 && is_model_supported(get_display_model())) {
            if (maxval >= 0x06) {
                registers[eax] = 0x06;
                cpuid(registers);
                // Bi-directional PROCHOT [0] and PTM [6]
                return (registers[eax] & (1 << 0)) && (registers[eax] & (1 << 6));
            }
        }
    }
    return false;
}

static bool eql_flag(const char *a, const char *b, size_t n)
{
    while (n > 0 && *a && *b) {
        if (*a != *b || *a == ':' || *b == ':') return false;
        ++a; ++b; --n;
    }
    return n == 0;
}

static bool has_flag(const char *args, const char *arg)
{
    if (arg[0] == '-' || arg[0] == '+') {
        size_t n = strlen(arg);
        for (const char *p = args; *p; ++p) {
            if ((p == args || p[-1] == ':') && (p[n] == 0 || p[n] == ':')
                    && eql_flag(p, arg, n)) {
                return true;
            }
        }
    }
    return false;
}

#define LOG(fmt, ...) IOLog("[GoodbyeBigSlow] " fmt "\n", __VA_ARGS__)

static void DBLog(const char *s)
{
    LOG("%s", s);
}

static void DBLogStatus(const char *s, int n)
{
    if (n == -1) {
        LOG("%s ...", s);
    } else {
        LOG("%s ... %s", s, n ? "Success" : "Failure");
    }
}

static kern_return_t kext_start(__unused kmod_info_t *_o, __unused void *data)
{
    if (!using_targeted_intel_cpu()) {
        DBLog("Targeted Intel CPU unavailable!");
        return KERN_FAILURE;
    }
    int ret = -1;

#define BOOT_ARGS_SIZE sizeof("-turbo:-speedstep")
    char boot_args[BOOT_ARGS_SIZE];

    if (PE_parse_boot_argn("GoodbyeBigSlow", &boot_args, BOOT_ARGS_SIZE)) {
        boot_args[BOOT_ARGS_SIZE - 1] = 0;
        if (has_flag(boot_args, "-turbo")) {
            DBLogStatus("Disabling Turbo Boost", -1);
            ret = disable_turbo();
            DBLogStatus("Disabling Turbo Boost", ret);
        }
        if (has_flag(boot_args, "-speedstep")) {
            DBLogStatus("Disabling SpeedStep", -1);
            ret = disable_speedstep();
            DBLogStatus("Disabling SpeedStep", ret);
        }
    }

    DBLogStatus("De-asserting Processor Hot", -1);
    ret = deassert_prochot();
    DBLogStatus("De-asserting Processor Hot", ret);

    return KERN_SUCCESS;
}

static kern_return_t kext_stop(__unused kmod_info_t *_o, __unused void *data)
{
    DBLog("Stopping ...");
    return KERN_SUCCESS;
}

#ifdef XCODE_OFF
static kern_return_t dummy(__unused kmod_info_t *_o, __unused void *data)
{
    return KERN_SUCCESS;
}
#ifndef KEXT_ID
#error KEXT_ID undefined
#endif
#ifndef KEXT_VERSION
#error KEXT_VERSION undefined
#endif
#define TO_STR(tokens) #tokens
extern kern_return_t _start(kmod_info_t *, void *);
extern kern_return_t _stop(kmod_info_t *, void *);
KMOD_EXPLICIT_DECL(KEXT_ID, TO_STR(KEXT_VERSION), _start, _stop)
// NOTE: should use kext_start and kext_stop if not providing IOService
__private_extern__ kmod_start_func_t *_realmain = dummy;
__private_extern__ kmod_stop_func_t  *_antimain = dummy;
__private_extern__ int _kext_apple_cc = __APPLE_CC__;
#endif // XCODE_OFF

#ifdef __cplusplus
} // extern "C"
#endif
