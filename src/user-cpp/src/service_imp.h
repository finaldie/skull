#ifndef SKULLCPP_SERVICE_IMP_H
#define SKULLCPP_SERVICE_IMP_H

#include <skull/service.h>
#include "skullcpp/service.h"

namespace skullcpp {

class ServiceImp : public Service {
private:
    skull_service_t* svc;

public:
    ServiceImp(skull_service_t*);
    virtual ~ServiceImp();

public:
    int createJob(Job job, void* ud, uint32_t delayed, int bioIdx);

public:
    void set(const void* data);

    void* get();
    const void* get() const;

public:
    skull_service_t* getRawService() const;
};

} // End of namespace

#endif

