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
    int createJob(uint32_t delayed, int bioIdx, Job job) const;

public:
    void set(const void* data);

    void* get();
    const void* get() const;

public:
    skull_service_t* getRawService() const;
};

// It's used by epclient callback
class ServiceApiDataImp : public ServiceApiData {
private:
    ServiceApiReqData*  req_;
    ServiceApiRespData* resp_;

public:
    ServiceApiDataImp();
    ServiceApiDataImp(ServiceApiReqData*, ServiceApiRespData*);
    virtual ~ServiceApiDataImp();

public:
    bool valid() const;

    const google::protobuf::Message& request() const;
    google::protobuf::Message& response() const;
};

} // End of namespace

#endif

