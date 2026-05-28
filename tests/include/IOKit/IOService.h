#ifndef MOCK_IOSERVICE_H
#define MOCK_IOSERVICE_H

#include <libkern/libkern.h>

class OSDictionary;
class OSObject {
public:
    virtual ~OSObject() {}
};

class IOService : public OSObject {
public:
    virtual bool init(OSDictionary* personality = 0) { return true; }
    virtual void free(void) {}
    virtual IOService* probe(IOService* provider, SInt32* score) { return this; }
    virtual bool start(IOService* provider) { return true; }
    virtual void stop(IOService* provider) {}
};

#define OSDefineMetaClassAndStructors(className, superName)
#define OSDeclareDefaultStructors(className)

#ifdef __cplusplus
extern "C" {
#endif
    void mp_rendezvous_no_intrs(void (*func)(void *), void *arg);
#ifdef __cplusplus
}
#endif

#endif
