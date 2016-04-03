#ifndef SKULLCPP_SERVICE_H
#define SKULLCPP_SERVICE_H

#include <stdint.h>
#include <google/protobuf/message.h>

#include <string>

#include <skull/config.h>
#include <skull/service.h>
#include <skullcpp/txn.h>

namespace skullcpp {

class Service {
private:
    // Make noncopyable
    Service(const Service& svc);
    const Service& operator=(const Service& svc);

public:
    typedef void (*Job) (Service&, void* ud);

public:
    Service() {};
    virtual ~Service() {};

public:
    virtual int createJob(Job job, void* ud, uint32_t delayed, int bioIdx) = 0;

public:
    virtual void set(const void* data) = 0;

    virtual void* get() = 0;
    virtual const void* get() const = 0;
};

} // End of namespace

#endif

