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
    /**
     * Create a service job
     *
     * @param job      Job callback function
     * @param ud       user data
     * @param delayed  unit milliseconds
     * @param bio_idx  background io idx
     *                  - (-1)  : random pick up one
     *                  - (0)   : do not use bio
     *                  - (> 0) : use the idx of bio
     *
     * @return 0 on success, 1 on failure
     */
    virtual int createJob(Job job, void* ud, uint32_t delayed, int bioIdx) const = 0;

public:
    virtual void set(const void* data) = 0;

    virtual void* get() = 0;
    virtual const void* get() const = 0;
};

class ServiceApiData {
private:
    // Make noncopyable
    ServiceApiData(const ServiceApiData&);
    const ServiceApiData& operator=(const ServiceApiData&);

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

