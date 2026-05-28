/*==========================================================================*\
)   Virtual Driver auto loaded at boot time.                                 (
\*==========================================================================*/

#include <IOKit/IOLib.h>
#include <i386/proc_reg.h>
#include "GoodbyeBigSlow.c"
#include "GoodbyeBigSlow.hpp"
#include "GoodbyeBigSlowShared.h"

OSDefineMetaClassAndStructors(GoodbyeBigSlow, IOService)
OSDefineMetaClassAndStructors(GoodbyeBigSlowUserClient, IOUserClient)
OSDefineMetaClassAndStructors(GoodbyeBigSlow_NoHardPLimits, GoodbyeBigSlow)

#define super IOService

bool GoodbyeBigSlow::init(OSDictionary* personality)
{
    DBLogStatus("Initializing", -1);
    const auto result = super::init(personality);
    DBLogStatus("Initializing", result ? 1 : 0);
    return result;
}

void GoodbyeBigSlow::free(void)
{
    DBLog("Freeing ...");
    super::free();
}

// Driver Matching: 1. IOProviderClass -> 2. personality -> 3. IOProbeScore
// Step 3: init() attach() probe() detach() // free() if probe fails
IOService* GoodbyeBigSlow::probe(IOService* provider, SInt32* score)
{
    DBLogStatus("Probing", -1);
    const auto result = super::probe(provider, score);
    DBLogStatus("Probing", result ? 1 : 0);
    return result;
}

bool GoodbyeBigSlow::start(IOService* provider)
{
    DBLogStatus("Starting", -1);
    const auto result = kext_start(NULL, NULL) == KERN_SUCCESS && super::start(provider);
    DBLogStatus("Starting", result ? 1 : 0);
    return result;
}

void GoodbyeBigSlow::stop(IOService* provider)
{
    kext_stop(NULL, NULL);
    super::stop(provider);
}

IOReturn GoodbyeBigSlow::newUserClient(task_t owningTask, void* securityID, UInt32 type, OSDictionary* properties, IOUserClient** handler)
{
    IOReturn result = kIOReturnSuccess;
    GoodbyeBigSlowUserClient* client = NULL;

    // Security check: Only allow root to open a user client
    if (IOUserClient::clientHasPrivilege(securityID, kIOClientPrivilegeAdministrator) != kIOReturnSuccess) {
        return kIOReturnNotPrivileged;
    }

    client = new GoodbyeBigSlowUserClient;
    if (!client) {
        return kIOReturnNoMemory;
    }

    if (!client->initWithTask(owningTask, securityID, type, properties)) {
        client->release();
        return kIOReturnInternalError;
    }

    if (!client->attach(this)) {
        client->release();
        return kIOReturnInternalError;
    }

    if (!client->start(this)) {
        client->detach(this);
        client->release();
        return kIOReturnInternalError;
    }

    *handler = client;
    return result;
}

#undef super
#define super IOUserClient

bool GoodbyeBigSlowUserClient::initWithTask(task_t owningTask, void* securityID, UInt32 type, OSDictionary* properties)
{
    if (!super::initWithTask(owningTask, securityID, type, properties)) {
        return false;
    }
    fProvider = NULL;
    return true;
}

bool GoodbyeBigSlowUserClient::start(IOService* provider)
{
    if (!super::start(provider)) {
        return false;
    }
    fProvider = OSDynamicCast(GoodbyeBigSlow, provider);
    return (fProvider != NULL);
}

void GoodbyeBigSlowUserClient::stop(IOService* provider)
{
    super::stop(provider);
}

IOReturn GoodbyeBigSlowUserClient::clientClose(void)
{
    terminate();
    return kIOReturnSuccess;
}

static bool is_safe_msr(uint32_t index)
{
    switch (index) {
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

IOReturn GoodbyeBigSlowUserClient::externalMethod(uint32_t selector, IOExternalMethodArguments* arguments, IOExternalMethodDispatch* dispatch, OSObject* target, void* reference)
{
    if (selector >= kGoodbyeBigSlowNumberOfMethods) {
        return kIOReturnUnsupported;
    }

    const GoodbyeBigSlowMSRArgs* input = (const GoodbyeBigSlowMSRArgs*)arguments->structureInput;
    GoodbyeBigSlowMSRArgs* output = (GoodbyeBigSlowMSRArgs*)arguments->structureOutput;

    if (!input || arguments->structureInputSize < sizeof(GoodbyeBigSlowMSRArgs)) {
        return kIOReturnBadArgument;
    }

    if (!is_safe_msr(input->index)) {
        return kIOReturnNotPrivileged;
    }

    switch (selector) {
        case kGoodbyeBigSlowReadMSR:
            if (!output || arguments->structureOutputSize < sizeof(GoodbyeBigSlowMSRArgs)) {
                return kIOReturnBadArgument;
            }
            output->index = input->index;
            output->value = rdmsr64(input->index);
            break;
        case kGoodbyeBigSlowWriteMSR:
            wrmsr64(input->index, input->value);
            break;
        default:
            return kIOReturnUnsupported;
    }

    return kIOReturnSuccess;
}
