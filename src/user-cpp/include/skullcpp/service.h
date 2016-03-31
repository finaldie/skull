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
    typedef enum Status {
        OK            = 0,
        ERROR_SRVNAME = 1,
        ERROR_APINAME = 2,
        ERROR_BIO     = 3
    } Status;

    typedef int (*ApiCB) (Txn&, const std::string& apiName,
                          const google::protobuf::Message& request,
                          const google::protobuf::Message& response);

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

/**
 * Invoke a service async call
 *
 * @param serivce_name
 * @param apiName
 * @param request       request protobuf message
 * @param cb            api callback function
 * @param bio_idx       background io index
 *                      - (-1)  : random pick up a background io to run
 *                      - (0)   : do not use background io
 *                      - (> 0) : run on the index of background io
 *
 * @return - OK
 *         - ERROR_SRVNAME
 *         - ERROR_APINAME
 *         - ERROR_BIO
 */
Service::Status ServiceCall (Txn&,
                             const char* serviceName,
                             const char* apiName,
                             const google::protobuf::Message& request,
                             Service::ApiCB cb,
                             int bio_idx);

} // End of namespace

#endif

