#ifndef SKULLCPP_UNITTEST_H
#define SKULLCPP_UNITTEST_H

#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <google/protobuf/message.h>

#include <skull/unittest.h>

namespace skullcpp {

class UTModule {
private:
    skullut_module_t* utModule_;
    std::string       idlName_;
    google::protobuf::Message* msg_;

public:
    UTModule(const std::string& moduleName, const std::string& idlName,
             const std::string& configName);
    ~UTModule();

public:
    typedef struct ServiceApi {
        const char* name;
        void (*iocall) (const google::protobuf::Message& request,
                        google::protobuf::Message& response);
    } ServiceApi;

public:
    bool run();
    bool addService(const std::string& svcName, ServiceApi** apis);

    void setTxnSharedData(const google::protobuf::Message&);
    google::protobuf::Message& getTxnSharedData();
    const google::protobuf::Message& getTxnSharedData() const;
};

class UTService {
private:
    std::string svcName_;
    skullut_service_t* utService_;

public:
    UTService(const std::string& svcName, const std::string& config);
    ~UTService();

public:
    void run(const std::string& apiName, const google::protobuf::Message& req,
             google::protobuf::Message& resp);

public:
    const std::string& svcName();
};

} // End of namespace

#endif

