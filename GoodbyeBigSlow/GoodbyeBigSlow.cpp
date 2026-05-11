/*==========================================================================*\
)   Virtual Driver auto loaded at boot time.                                 (
\*==========================================================================*/

#include <IOKit/IOLib.h>
#include "GoodbyeBigSlow.c"
#include "GoodbyeBigSlow.hpp"

OSDefineMetaClassAndStructors(GoodbyeBigSlow, IOService)

#define super IOService

OSDefineMetaClassAndStructors(GoodbyeBigSlowUserClient, IOUserClient)

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
    GoodbyeBigSlowUserClient* client = new GoodbyeBigSlowUserClient;
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

#undef super
#define super IOUserClient

bool GoodbyeBigSlowUserClient::initWithTask(task_t owningTask, void* securityID, UInt32 type, OSDictionary* properties)
{
    if (!super::initWithTask(owningTask, securityID, type, properties)) return false;
    fProvider = NULL;
    return true;
}

bool GoodbyeBigSlowUserClient::start(IOService* provider)
{
    if (!super::start(provider)) return false;
    fProvider = OSDynamicCast(GoodbyeBigSlow, provider);
    return fProvider != NULL;
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

IOReturn GoodbyeBigSlowUserClient::externalMethod(uint32_t selector, IOExternalMethodArguments* arguments, IOExternalMethodDispatch* dispatch, OSObject* target, void* reference)
{
    if (selector == 0) { // Read MSR (restricted)
        if (arguments->scalarInputCount != 1 || arguments->scalarOutputCount != 1) return kIOReturnBadArgument;
        uint32_t msr = (uint32_t)arguments->scalarInput[0];
        // Only allow reading specific safe MSRs to avoid GPF/Panic
        switch (msr) {
            case 0x198: // MSR_IA32_PERF_STS
            case 0x199: // MSR_IA32_PERF_CTL
            case 0x1B1: // MSR_IA32_PACKAGE_THERM_STATUS
            case 0x19C: // MSR_IA32_THERM_STATUS
            case 0x1FC: // MSR_IA32_POWER_CTL
            case 0x1A0: // MSR_IA32_MISC_ENABLE
            case 0x017: // MSR_IA32_PLATFORM_ID
                arguments->scalarOutput[0] = rdmsr64(msr);
                return kIOReturnSuccess;
            default:
                return kIOReturnNotPrivileged;
        }
    }
    return kIOReturnUnsupported;
}
