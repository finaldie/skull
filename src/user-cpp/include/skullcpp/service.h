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
    typedef std::function<void (Service&, ServiceApiData&)> JobW;
    typedef std::function<void (const Service&, ServiceApiData&)> JobR;
    typedef JobR JobError;

    typedef std::function<void (Service&)> JobNPW;
    typedef std::function<void (const Service&)> JobNPR;
    typedef JobNPR JobNPError;

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
    virtual int createJob(JobR, JobError = nullptr) const = 0;
    virtual int createJob(JobW, JobError = nullptr) const = 0;

    /**
     * Create a pending service job
     *
     * @param delayed  unit milliseconds
     * @param job      Job callback function
     *
     * @return 0 on success, 1 on failure
     */
    virtual int createJob(uint32_t delayed, JobR, JobError = nullptr) const = 0;
    virtual int createJob(uint32_t delayed, JobW, JobError = nullptr) const = 0;

    /**
     * Create a no pending service job
     *
     * @param delayed  unit milliseconds
     * @param interval unit milliseconds. 0: not a repeated job
     * @param bio_idx  background io idx
     *                  - (-1)  : random pick up one
     *                  - (0)   : do not use bio
     *                  - (> 0) : use the idx of bio
     * @param job      Job callback function
     *
     * @return 0 on success, 1 on failure
     */
    virtual int createJob(uint32_t delayed, uint32_t interval, int bioIdx, JobNPR, JobNPError = nullptr) const = 0;
    virtual int createJob(uint32_t delayed, uint32_t interval, int bioIdx, JobNPW, JobNPError = nullptr) const = 0;

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
    virtual int createJob(uint32_t delayed, int bioIdx, JobNPR, JobNPError = nullptr) const = 0;
    virtual int createJob(uint32_t delayed, int bioIdx, JobNPW, JobNPError = nullptr) const = 0;

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
    virtual int createJob(int bioIdx, JobNPR, JobNPError = nullptr) const = 0;
    virtual int createJob(int bioIdx, JobNPW, JobNPError = nullptr) const = 0;

    /**
     * Create a no pending service job
     *
     * @param delayed  unit milliseconds
     * @param job      Job callback function
     *
     * @return 0 on success, 1 on failure
     */
    virtual int createJob(uint32_t delayed, JobNPR, JobNPError = nullptr) const = 0;
    virtual int createJob(uint32_t delayed, JobNPW, JobNPError = nullptr) const = 0;

    /**
     * Create a no pending service job, it will be executed ASAP
     *
     * @param job      Job callback function
     *
     * @return 0 on success, 1 on failure
     */
    virtual int createJob(JobNPR, JobNPError = nullptr) const = 0;
    virtual int createJob(JobNPW, JobNPError = nullptr) const = 0;

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
    ServiceData() = default;
    virtual ~ServiceData() = default;
};

// [Deprecated Macros]
// Use these macros to create a Service::Job when the compiler doesn't fully
//  support c++11 not very well. Prefer to use lambda function instead.
#define skull_BindSvcJobR(f, ...) \
    (skullcpp::Service::JobR) \
    std::bind(f, std::placeholders::_1, std::placeholders::_2, ##__VA_ARGS__)

#define skull_BindSvcJobW(f, ...) \
    (skullcpp::Service::JobW) \
    std::bind(f, std::placeholders::_1, std::placeholders::_2, ##__VA_ARGS__)

#define skull_BindSvcJobError skull_BindSvcJobR

#define skull_BindSvcJobNPR(f, ...) \
    (skullcpp::Service::JobNPR) \
    std::bind(f, std::placeholders::_1, ##__VA_ARGS__)

#define skull_BindSvcJobNPW(f, ...) \
    (skullcpp::Service::JobNPW) \
    std::bind(f, std::placeholders::_1, ##__VA_ARGS__)

#define skull_BindSvcJobNPError skull_BindSvcJobNPR

} // End of namespace

#endif

