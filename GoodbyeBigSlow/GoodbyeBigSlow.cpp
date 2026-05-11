/*==========================================================================*\
)   Virtual Driver auto loaded at boot time.                                 (
\*==========================================================================*/

#include <IOKit/IOLib.h>
#include "GoodbyeBigSlowShared.h"
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

IOReturn GoodbyeBigSlow::newUserClient(task_t owningTask, void* securityID,
                                        UInt32 type, OSDictionary* properties,
                                        IOUserClient** handler)
{
    IOReturn ret = kIOReturnSuccess;
    GoodbyeBigSlowClient* client = new GoodbyeBigSlowClient;

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
    return ret;
}

OSDefineMetaClassAndStructors(GoodbyeBigSlowClient, IOUserClient)

bool GoodbyeBigSlowClient::initWithTask(task_t owningTask, void* securityID,
                                         UInt32 type, OSDictionary* properties)
{
    if (!super::initWithTask(owningTask, securityID, type, properties)) return false;
    fTask = owningTask;
    return true;
}

bool GoodbyeBigSlowClient::start(IOService* provider)
{
    fProvider = OSDynamicCast(GoodbyeBigSlow, provider);
    if (!fProvider) return false;
    if (!super::start(provider)) return false;
    return true;
}

void GoodbyeBigSlowClient::stop(IOService* provider)
{
    super::stop(provider);
}

IOReturn GoodbyeBigSlowClient::clientClose(void)
{
    terminate();
    return kIOReturnSuccess;
}

IOReturn GoodbyeBigSlowClient::externalMethod(uint32_t selector, IOExternalMethodArguments* arguments,
                                               IOExternalMethodDispatch* dispatch, OSObject* target, void* reference)
{
    if (selector >= kGBSMethodCount) return kIOReturnBadArgument;

    // Dispatch table for external methods
    static const IOExternalMethodDispatch sMethods[kGBSMethodCount] = {
        {(IOExternalMethodAction)&gbs_msr_read, 1, 0, 1, 0},
        {(IOExternalMethodAction)&gbs_msr_write, 4, 0, 0, 0},
        {(IOExternalMethodAction)&gbs_plimit, 1, 0, 0, 0},
        {(IOExternalMethodAction)&gbs_gpu_frequency, 1, 0, 0, 0},
        {(IOExternalMethodAction)&gbs_cpu_frequency, 1, 0, 0, 0},
        {(IOExternalMethodAction)&gbs_cpu_idle, 1, 0, 0, 0},
        {(IOExternalMethodAction)&gbs_cpu_hwp, 1, 0, 0, 0},
        {(IOExternalMethodAction)&gbs_voltage, 2, 0, 0, 0},
    };

    return sMethods[selector].checkAndCall(this, target, arguments);
}
