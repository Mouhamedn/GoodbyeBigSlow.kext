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

#ifndef MSR_IA32_PERF_STS
#define MSR_IA32_PERF_STS 0x198
#endif
#ifndef MSR_IA32_PERF_CTL
#define MSR_IA32_PERF_CTL 0x199
#endif
#ifndef IA32_PM_ENABLE
#define IA32_PM_ENABLE 0x770
#endif
#ifndef IA32_HWP_REQUEST
#define IA32_HWP_REQUEST 0x774
#endif

// https://developer.apple.com/documentation/kernel/1576460-osincrementatomic
// https://lwn.net/Articles/793253/
#define VOLATILE_ACCESS(x) (*((volatile __typeof__(x) *)&(x)))

#ifndef MSR_IA32_POWER_CTL
#define MSR_IA32_POWER_CTL 0x1FC
#endif
// Credit to https://www.techpowerup.com/download/techpowerup-throttlestop/
const uint64_t kMsrEnableProcHot = 1;

#ifndef MSR_IA32_MISC_ENABLE
#define MSR_IA32_MISC_ENABLE 0x1A0
#endif
const uint64_t kMsrEnableSpeedStep = 1ULL << 16;
const uint64_t kMsrDisableTurboBoost = 1ULL << 38;

#ifndef MSR_IA32_PACKAGE_THERM_STATUS
#define MSR_IA32_PACKAGE_THERM_STATUS 0x1B1
#endif
#ifndef MSR_IA32_THERM_STATUS // per core
#define MSR_IA32_THERM_STATUS 0x19C
#endif
const uint64_t kMsrThermalStatusMask = 0x28A;  // 0b1010001010

// https://github.com/apple/darwin-xnu/blob/main/osfmk/i386/mp.h
// Perform actions on all processor cores.
extern void mp_rendezvous_no_intrs(void (*func)(void *), void *arg);

// ALERT: Toggling PROCHOT more than once in ~2 ms period can result in
//        constant Pn state (Low Frequency Mode) of the processor.
static void mp_deassert_prochot(void *data)
{
    SInt64 *cpucount = (SInt64 *)data;
    SInt64 *disabled = (SInt64 *)data + 1;
    uint64_t old_bits = rdmsr64(MSR_IA32_POWER_CTL);
    uint64_t new_bits = old_bits & ~kMsrEnableProcHot;

    // hopefully not to cause invalid write and the black screen of death
    if (old_bits != new_bits) {
        wrmsr64(MSR_IA32_POWER_CTL, new_bits);
        IOSleep(1);
        if (!(rdmsr64(MSR_IA32_POWER_CTL) & kMsrEnableProcHot)) {
            OSIncrementAtomic64(VOLATILE_ACCESS(disabled));
        }
    } else {
        OSIncrementAtomic64(VOLATILE_ACCESS(disabled));
    }
    OSIncrementAtomic64(VOLATILE_ACCESS(cpucount));
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
            // XXX: CPUID.06H:EAX[1] => 0
            wrmsr64(MSR_IA32_MISC_ENABLE, new_bits);
            IOSleep(1);
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
        uint64_t new_bits = old_bits & ~kMsrEnableSpeedStep;

        if (old_bits != new_bits) {
            wrmsr64(MSR_IA32_MISC_ENABLE, new_bits);
            IOSleep(1);
            return !(rdmsr64(MSR_IA32_MISC_ENABLE) & kMsrEnableSpeedStep);
        }
    }
    return true;
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

static bool using_targeted_intel_cpu(void)
{
    uint32_t registers[4] = {[eax]=0x00, [ebx]=0xFF, [ecx]=0xFF, [edx]=0xFF};
    cpuid(registers);
    uint32_t maxval = registers[eax];

    bool GenuineIntel = registers[ebx] == 0x756E6547 &&
                        registers[edx] == 0x49656E69 &&
                        registers[ecx] == 0x6C65746E &&
                        maxval >= 0x01;

    // check cpu vendor
    if (GenuineIntel) {
        registers[eax] = 0x01;
        cpuid(registers);
        // check cpu family but not model
        if (((registers[eax] >> 8) & 0x0F) == 0x06) {
            // supports package thermal management (PTM) ?
            if (maxval >= 0x06) {
                registers[eax] = 0x06;
                cpuid(registers);
                return registers[eax] & (1 << 6);
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

    // GoodbyeBigSlow=<flags>  // "-": disable; "+": enable
#define BOOT_ARGS_SIZE sizeof("-turbo:-speedstep")
    // https://github.com/apple/darwin-xnu/blob/main/pexpert/gen/bootargs.c
    // XXX: better use own parser if this kext is needed for macos < v10.11.6
    // PE_parse_boot_argn(<key>, arg_ptr, arg_size) => matched?
    //     argument (the part before "=" is compared with <key>)
    //         boolean: (?P<key>-[^\x09\x20=]*)(=[^\x09\x20]*)?
    //         key=val: (?P<key>[^\x09\x20\-=]*)=(?P<val>[^\x09\x20]*)
    //         skipped: not -* nor *=*
    // if boot-arg is boolean
    // then *(intN_t *)arg_ptr = 1
    // else if <key> begins with "_"
    // then strlcpy(arg_ptr, <val>, max(16, arg_size - 1))  // forced & unsafe
    // else if <val> is (+/-) 0xHHHH... 0b1010... 0755... (k/K/m/M/g/G)
    // then *(intN_t *)arg_ptr = integer(<val>)  // N = max(arg_size * 8, 64)
    // else strlcpy(arg_ptr, <val>, arg_size - 1)
    // return true after the first match; no copy/assignment if arg_size is 0
    char boot_args[BOOT_ARGS_SIZE];

    // FIXME: CPU model and MSR read/write permission not checked
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

struct msr_args {
    uint32_t addr;
    uint64_t value;
    uint32_t op;
};

static void mp_msr_write(void *data)
{
    struct msr_args *args = (struct msr_args *)data;
    uint64_t val = args->value;
    if (args->op != kGBSMSROpWrite) {
        uint64_t old = rdmsr64(args->addr);
        if (args->op == kGBSMSROpAnd) val = old & args->value;
        else if (args->op == kGBSMSROpOr) val = old | args->value;
        else if (args->op == kGBSMSROpXor) val = old ^ args->value;
    }
    wrmsr64(args->addr, val);
}

// External method actions
static kern_return_t gbs_msr_read(void *target, void *reference, IOExternalMethodArguments *arguments)
{
    uint32_t addr = (uint32_t)arguments->scalarInput[0];
    arguments->scalarOutput[0] = rdmsr64(addr);
    return KERN_SUCCESS;
}

static kern_return_t gbs_msr_write(void *target, void *reference, IOExternalMethodArguments *arguments)
{
    struct msr_args args;
    args.addr = (uint32_t)arguments->scalarInput[0];
    args.value = arguments->scalarInput[1];
    args.op = (uint32_t)arguments->scalarInput[2];
    uint32_t mode = (uint32_t)arguments->scalarInput[3];

    if (mode == kGBSMSRModeAll) {
        mp_rendezvous_no_intrs(mp_msr_write, &args);
    } else {
        mp_msr_write(&args);
    }
    return KERN_SUCCESS;
}

static kern_return_t gbs_plimit(void *target, void *reference, IOExternalMethodArguments *arguments)
{
    // Mock implementation for plimit
    // 0x199 MSR_IA32_PERF_CTL
    uint64_t frequency = arguments->scalarInput[0];
    struct msr_args args = {MSR_IA32_PERF_CTL, (frequency / 100) << 8, kGBSMSROpWrite};
    mp_rendezvous_no_intrs(mp_msr_write, &args);
    return KERN_SUCCESS;
}

static kern_return_t gbs_gpu_frequency(void *target, void *reference, IOExternalMethodArguments *arguments)
{
    // GPU frequency control on some Intel CPUs (e.g. Broadwell+)
    // can be influenced by MSR_PKG_POWER_LIMIT (0x610)
    // but usually it's handled via MCHBAR MMIO or specific MSRs if available.
    // For now, we provide a placeholder that could be extended.
    uint64_t frequency = arguments->scalarInput[0];
    (void)frequency;
    return KERN_SUCCESS;
}

static kern_return_t gbs_cpu_frequency(void *target, void *reference, IOExternalMethodArguments *arguments)
{
    uint64_t frequency = arguments->scalarInput[0];
    // Convert MHz to ratio (100MHz bus)
    uint64_t ratio = frequency / 100;
    struct msr_args args = {MSR_IA32_PERF_CTL, (ratio << 8), kGBSMSROpWrite};
    mp_rendezvous_no_intrs(mp_msr_write, &args);
    return KERN_SUCCESS;
}

static kern_return_t gbs_cpu_idle(void *target, void *reference, IOExternalMethodArguments *arguments)
{
    // Package C-State limit control (0xE2)
    // Bits 2:0 - PKG C-state limit.
    uint64_t percent = arguments->scalarInput[0];
    uint32_t c_state_limit = 0;
    if (percent == 0) c_state_limit = 0; // No limit
    else if (percent < 50) c_state_limit = 2; // e.g. C2
    else c_state_limit = 7; // No limit or deeper states

    struct msr_args args = {0xE2, c_state_limit, kGBSMSROpOr};
    mp_rendezvous_no_intrs(mp_msr_write, &args);
    return KERN_SUCCESS;
}

static kern_return_t gbs_cpu_hwp(void *target, void *reference, IOExternalMethodArguments *arguments)
{
    uint64_t frequency = arguments->scalarInput[0];
    // Enable HWP if not enabled
    uint64_t enabled = rdmsr64(IA32_PM_ENABLE);
    if (!(enabled & 1)) {
        wrmsr64(IA32_PM_ENABLE, 1);
    }
    // Set HWP Request
    // Bits 7:0 minimum, 15:8 maximum, 23:16 desired
    uint64_t ratio = frequency / 100;
    uint64_t val = (ratio << 16) | (ratio << 8) | ratio;
    struct msr_args args = {IA32_HWP_REQUEST, val, kGBSMSROpWrite};
    mp_rendezvous_no_intrs(mp_msr_write, &args);
    return KERN_SUCCESS;
}

static kern_return_t gbs_voltage(void *target, void *reference, IOExternalMethodArguments *arguments)
{
    // Voltage control via MSR 0x150 (OC_MAILBOX)
    // This is highly dangerous and model-specific.
    // Basic interface for Haswell+
    uint64_t min_vol = arguments->scalarInput[0]; // in mV
    uint64_t max_vol = arguments->scalarInput[1]; // in mV
    (void)min_vol; (void)max_vol;

    // Example sequence (not active for safety):
    // 1. Write command to 0x150
    // uint64_t cmd = ...;
    // wrmsr64(0x150, cmd);

    return KERN_SUCCESS;
}

#ifdef __cplusplus
} // extern "C"
#endif
