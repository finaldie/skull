#ifndef SKULLCPP_SERVICE_H
#define SKULLCPP_SERVICE_H

#include <stdint.h>
#include <google/protobuf/message.h>

#include <string>
#include <memory>
#include <functional>

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
    typedef std::function<void (Service&)> Job;

public:
    Service() {};
    virtual ~Service() {};

public:
    /**
     * Create a service job
     *
     * @param delayed  unit milliseconds
     * @param bio_idx  background io idx
     *                  - (-1)  : random pick up one
     *                  - (0)   : do not use bio
     *                  - (> 0) : use the idx of bio
     * @param job      Job callback function
     *
     * @return 0 on success, 1 on failure
     */
    virtual int createJob(uint32_t delayed, int bioIdx, Job) const = 0;

// Use this macro to make a Service::Job easily
#define skull_BindSvc(f, ...) \
    std::bind(f, std::placeholders::_1, ##__VA_ARGS__)

public:
    virtual void set(const void* data) = 0;

    virtual void* get() = 0;
    virtual const void* get() const = 0;
};

class ServiceApiData {
public:
    ServiceApiData() {}
    virtual ~ServiceApiData() {}

public:
    virtual bool valid() const = 0;

    virtual const google::protobuf::Message& request() const = 0;

    virtual google::protobuf::Message& response() const = 0;
};

} // End of namespace

#endif

