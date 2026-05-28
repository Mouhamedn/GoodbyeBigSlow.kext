/*==========================================================================*\
)   Virtual Driver auto loaded at boot time.                                 (
\*==========================================================================*/

#include <IOKit/IOLib.h>
#include <IOKit/IOUserClient.h>
#include "GoodbyeBigSlow.c"
#include "GoodbyeBigSlow.hpp"

OSDefineMetaClassAndStructors(GoodbyeBigSlow, IOService)

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
    const auto result = using_targeted_intel_cpu() ? super::probe(provider, score) : NULL;
    DBLogStatus("Probing", result ? 1 : 0);
    return result;
}

bool GoodbyeBigSlow::start(IOService* provider)
{
    DBLogStatus("Starting", -1);
    const auto result = kext_start(NULL, NULL) == KERN_SUCCESS && super::start(provider);
    if (result) {
        registerService();
    }
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
    if (IOUserClient::clientHasPrivilege(owningTask, kIOClientPrivilegeAdministrator) != kIOReturnSuccess) {
        return kIOReturnNotPrivileged;
    }

    GoodbyeBigSlowUserClient* client = OSTypeAlloc(GoodbyeBigSlowUserClient);
    if (!client) return kIOReturnNoMemory;

    if (!client->initWithTask(owningTask, securityID, type, properties)) {
        client->release();
        return kIOReturnError;
    }

    if (!client->attach(this)) {
        client->release();
        return kIOReturnError;
    }

    if (!client->start(this)) {
        client->detach(this);
        client->release();
        return kIOReturnError;
    }

    *handler = client;
    return kIOReturnSuccess;
}

OSDefineMetaClassAndStructors(GoodbyeBigSlowUserClient, IOUserClient)

bool GoodbyeBigSlowUserClient::initWithTask(task_t owningTask, void* securityID, UInt32 type, OSDictionary* properties)
{
    if (!IOUserClient::initWithTask(owningTask, securityID, type, properties)) return false;
    fTask = owningTask;
    return true;
}

bool GoodbyeBigSlowUserClient::start(IOService* provider)
{
    if (!IOUserClient::start(provider)) return false;
    return true;
}

void GoodbyeBigSlowUserClient::stop(IOService* provider)
{
    IOUserClient::stop(provider);
}

IOReturn GoodbyeBigSlowUserClient::clientClose(void)
{
    terminate();
    return kIOReturnSuccess;
}

IOReturn GoodbyeBigSlowUserClient::externalMethod(uint32_t selector, IOExternalMethodArguments* arguments, IOExternalMethodDispatch* dispatch, OSObject* target, void* reference)
{
    (void)dispatch;
    (void)target;
    (void)reference;

    if (selector == 0) { // rdmsr64
        if (arguments->scalarInputCount < 1 || arguments->scalarOutputCount < 1) return kIOReturnBadArgument;
        uint32_t msr = (uint32_t)arguments->scalarInput[0];
        if (!is_msr_allowed(msr)) return kIOReturnNotPermitted;
        arguments->scalarOutput[0] = rdmsr64(msr);
        return kIOReturnSuccess;
    } else if (selector == 1) { // wrmsr64
        if (arguments->scalarInputCount < 2) return kIOReturnBadArgument;
        uint32_t msr = (uint32_t)arguments->scalarInput[0];
        uint64_t val = arguments->scalarInput[1];
        if (!is_msr_allowed(msr)) return kIOReturnNotPermitted;
        wrmsr_all(msr, val);
        return kIOReturnSuccess;
    }
    return kIOReturnBadArgument;
}

OSDefineMetaClassAndStructors(GoodbyeBigSlow_NoHardPLimits, IOService)

bool GoodbyeBigSlow_NoHardPLimits::start(IOService* provider)
{
    if (!IOService::start(provider)) return false;
    registerService();
    return true;
}
