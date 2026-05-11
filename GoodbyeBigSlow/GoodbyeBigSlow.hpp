#include <IOKit/IOService.h>
#include <IOKit/IOUserClient.h>

class GoodbyeBigSlow : public IOService
{
OSDeclareDefaultStructors(GoodbyeBigSlow)
public:
    virtual bool init(OSDictionary* personality = 0) override;
    virtual void free(void) override;
    virtual IOService* probe(IOService* provider, SInt32* score) override;
    virtual bool start(IOService* provider) override;
    virtual void stop(IOService* provider) override;
    virtual IOReturn newUserClient(task_t owningTask, void* securityID, UInt32 type, OSDictionary* properties, IOUserClient** handler) override;
};

class GoodbyeBigSlowUserClient : public IOUserClient
{
OSDeclareDefaultStructors(GoodbyeBigSlowUserClient)
public:
    virtual bool initWithTask(task_t owningTask, void* securityID, UInt32 type, OSDictionary* properties) override;
    virtual bool start(IOService* provider) override;
    virtual void stop(IOService* provider) override;
    virtual IOReturn clientClose(void) override;
    virtual IOReturn externalMethod(uint32_t selector, IOExternalMethodArguments* arguments, IOExternalMethodDispatch* dispatch, OSObject* target, void* reference) override;

private:
    GoodbyeBigSlow* fProvider;
};
