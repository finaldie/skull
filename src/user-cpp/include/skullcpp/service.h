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

class ServiceApiData;
class ServiceData;

class Service {
private:
    // Make noncopyable
    Service(const Service& svc) = delete;
    Service(Service&& svc) = delete;
    Service& operator=(const Service& svc) = delete;
    Service& operator=(Service&& svc) = delete;

public:
    typedef std::function<void (Service&, ServiceApiData&)> Job;
    typedef std::function<void (Service&)> JobNP;

public:
    Service() {};
    virtual ~Service() {};

public:
    /**
     * Create a pending service job, which will be triggered ASAP
     *
     * @param job      Job callback function
     *
     * @return 0 on success, 1 on failure
     */
    virtual int createJob(Job) const = 0;

    /**
     * Create a pending service job
     *
     * @param delayed  unit milliseconds
     * @param job      Job callback function
     *
     * @return 0 on success, 1 on failure
     */
    virtual int createJob(uint32_t delayed, Job) const = 0;

    /**
     * Create a no pending service job
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
    virtual int createJob(uint32_t delayed, int bioIdx, JobNP) const = 0;

    /**
     * Create a no pending service job, which will be triggered ASAP
     *
     * @param bio_idx  background io idx
     *                  - (-1)  : random pick up one
     *                  - (0)   : do not use bio
     *                  - (> 0) : use the idx of bio
     * @param job      Job callback function
     *
     * @return 0 on success, 1 on failure
     */
    virtual int createJob(int bioIdx, JobNP) const = 0;

    /**
     * Create a no pending service job
     *
     * @param delayed  unit milliseconds
     * @param job      Job callback function
     *
     * @return 0 on success, 1 on failure
     */
    virtual int createJob(uint32_t delayed, JobNP) const = 0;

    /**
     * Create a no pending service job
     *
     * @param job      Job callback function
     *
     * @return 0 on success, 1 on failure
     */
    virtual int createJob(JobNP) const = 0;

// Use these macros to make a Service::Job easily, you still can use lambda
//  instead of them
#define skull_BindSvcJob(f, ...) \
    (skullcpp::Service::Job) \
    std::bind(f, std::placeholders::_1, std::placeholders::_2, ##__VA_ARGS__)

#define skull_BindSvcJobNP(f, ...) \
    (skullcpp::Service::JobNP) \
    std::bind(f, std::placeholders::_1, ##__VA_ARGS__)

public:
    virtual void set(ServiceData* data) = 0;

    virtual ServiceData* get() = 0;
    virtual const ServiceData* get() const = 0;
};

class ServiceApiData {
private:
    // Make noncopyable
    ServiceApiData(const ServiceApiData&) = delete;
    ServiceApiData(ServiceApiData&&) = delete;
    ServiceApiData& operator=(const ServiceApiData&) = delete;
    ServiceApiData& operator=(ServiceApiData&&) = delete;

public:
    ServiceApiData() {}
    virtual ~ServiceApiData() {}

public:
    virtual bool valid() const = 0;

    virtual const google::protobuf::Message& request() const = 0;

    virtual google::protobuf::Message& response() const = 0;
};

class ServiceData {
private:
    // Make noncopyable
    ServiceData(const ServiceData&) = delete;
    ServiceData(ServiceData&&) = delete;
    ServiceData& operator=(const ServiceData&) = delete;
    ServiceData& operator=(ServiceData&&) = delete;

public:
    ServiceData() {}
    virtual ~ServiceData() {}
};

} // End of namespace

#endif

