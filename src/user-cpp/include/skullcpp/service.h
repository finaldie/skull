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
    skull_service_t* svc;

public:
    typedef void (*Job) (Service&, void* ud);

public:
    Service(skull_service_t*);
    ~Service();

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

