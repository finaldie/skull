#ifndef SKULLCPP_UNITTEST_H
#define SKULLCPP_UNITTEST_H

#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <map>
#include <vector>
#include <google/protobuf/message.h>

#include <skull/unittest.h>

namespace skullcpp {

class UTModule {
private:
    typedef std::vector<const google::protobuf::Message*> ApiResponseQueue;
    typedef struct ApiResponses {
        ApiResponseQueue queue;
        size_t idx;

        ApiResponses() : idx(0) {}
    } ApiResponses;

    typedef std::map<std::string, ApiResponses> MockApiCall;
    typedef std::map<std::string, MockApiCall*> MockSvcs;

private:
    skullut_module_t* utModule_;
    std::string       idlName_;
    google::protobuf::Message* msg_;
    MockSvcs          mockSvcs_;

public:
    UTModule(const std::string& moduleName, const std::string& idlName,
             const std::string& configName);
    ~UTModule();

public:
    bool run();

    bool pushServiceCall(const std::string& svcName, const std::string& apiName,
                         const google::protobuf::Message& response);
    const google::protobuf::Message* popServiceCall(const std::string& svcName,
                                                    const std::string& apiName);

    void setTxnSharedData(const google::protobuf::Message&);
    google::protobuf::Message& getTxnSharedData();
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
    const std::string& name() const;
};

} // End of namespace

#endif

