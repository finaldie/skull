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
    int createJob(uint32_t delayed, JobR, JobError) const;
    int createJob(uint32_t delayed, JobW, JobError) const;
    int createJob(JobR, JobError) const;
    int createJob(JobW, JobError) const;

    int createJob(uint32_t delayed, int bioIdx, JobNPR, JobNPError) const;
    int createJob(uint32_t delayed, int bioIdx, JobNPW, JobNPError) const;
    int createJob(int bioIdx, JobNPR, JobNPError) const;
    int createJob(int bioIdx, JobNPW, JobNPError) const;
    int createJob(uint32_t delayed, JobNPR, JobNPError) const;
    int createJob(uint32_t delayed, JobNPW, JobNPError) const;
    int createJob(JobNPR, JobNPError) const;
    int createJob(JobNPW, JobNPError) const;

public:
    void set(ServiceData* data);

    ServiceData* get();
    const ServiceData* get() const;

public:
    skull_service_t* getRawService() const;
};

// It's used by epclient callback
// Notes: It only can be used for pending EP call, otherwise the program may
//  crashed
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

