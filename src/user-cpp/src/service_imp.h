#ifndef SKULLCPP_SERVICE_IMP_H
#define SKULLCPP_SERVICE_IMP_H

#include <skull/service.h>

#include "srv_utils.h"
#include "skullcpp/service.h"

namespace skullcpp {

class ServiceImp : public Service {
private:
    skull_service_t* svc;

public:
    ServiceImp(skull_service_t*);
    virtual ~ServiceImp();

public:
    int createJob(Job job) const;
    int createJob(uint32_t delayed, Job job) const;

    int createJob(uint32_t delayed, int bioIdx, JobNP job) const;
    int createJob(uint32_t delayed, JobNP job) const;
    int createJob(int bioIdx, JobNP job) const;
    int createJob(JobNP job) const;

public:
    void set(ServiceData* data);

    ServiceData* get();
    const ServiceData* get() const;

public:
    skull_service_t* getRawService() const;
};

// It's used by epclient callback
class ServiceApiDataImp : public ServiceApiData {
private:
    ServiceApiReqData*  req_;
    ServiceApiRespData* resp_;
    bool cleanup;

    // padding fields
    bool  __padding;
    short __padding1;

#if __WORDSIZE == 64
    int   __padding2;
#endif

public:
    ServiceApiDataImp();
    ServiceApiDataImp(ServiceApiReqData*, ServiceApiRespData*);
    ServiceApiDataImp(skull_service_t*, const void* apiReq, size_t apiReqSz,
                      void* apiResp, size_t apiRespSz);
    ServiceApiDataImp(const skull_service_t*, const void* apiReq, size_t apiReqSz,
                      void* apiResp, size_t apiRespSz);
    virtual ~ServiceApiDataImp();

public:
    bool valid() const;

    const google::protobuf::Message& request() const;
    google::protobuf::Message& response() const;
};

} // End of namespace

#endif

