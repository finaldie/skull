#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <map>
#include <google/protobuf/descriptor.h>

#include "module_loader.h"
#include "srv_loader.h"
#include "srv_utils.h"
#include "txn_idldata.h"

#include "skullcpp/unittest.h"

namespace skullcpp {

using namespace google::protobuf;

UTModule::UTModule(const std::string& moduleName, const std::string& idlName,
                   const std::string& configName) {
    // 1. Create skullut_module env
    skull_module_loader_t loader = module_getloader();
    this->utModule_ = skullut_module_create(moduleName.c_str(), idlName.c_str(),
                                            configName.c_str(), "cpp", loader);

    // 2. fill basic fields
    this->idlName_ = idlName;
    this->msg_     = NULL;
}

UTModule::~UTModule() {
    if (this->msg_) {
        delete this->msg_;
        this->msg_ = NULL;
    }

    TxnSharedRawData* rawData =
        (TxnSharedRawData*)skullut_module_data(this->utModule_);
    delete rawData;

    skullut_module_destroy(this->utModule_);
}

bool UTModule::run() {
    return skullut_module_run(this->utModule_);
}

class MockSvcData {
private:
    std::map<std::string, UTModule::ServiceApi*> apis_;

public:
    MockSvcData(UTModule::ServiceApi** apis) {
        for (int i = 0; apis[i] != NULL; i++) {
            this->apis_.insert(std::make_pair(std::string(apis[i]->name), apis[i]));
        }
    }

    ~MockSvcData() {}

    const UTModule::ServiceApi* getApi(const char* apiName) const {
        std::map<std::string, UTModule::ServiceApi*>::const_iterator it;

        it = this->apis_.find(apiName);
        if (it != this->apis_.end()) {
            return it->second;
        } else {
            return NULL;
        }
    }
};

// Fill the task.response
static
void _iocall(const char* apiName, skullmock_task_t* task, void* ud) {
    MockSvcData* mockSvcData = (MockSvcData*)ud;
    const UTModule::ServiceApi* svcApi = mockSvcData->getApi(apiName);

    // prepare apiReq and apiResp
    ServiceApiReqData apiReq(task);
    ServiceApiRespData apiResp(task->service->name, task->api_name, task->response,
                               task->response_sz);
    google::protobuf::Message& respMsg = apiResp.get();

    svcApi->iocall(apiReq.get(), respMsg);

    // Fill task.response
    task->response_sz = (size_t)respMsg.ByteSize();
    task->response = calloc(1, task->response_sz);
    respMsg.SerializeToArray(task->response, (int)task->response_sz);
}

static
void _release(void* ud) {
    MockSvcData* mockSvcData = (MockSvcData*)ud;
    delete(mockSvcData);
}

bool UTModule::addService(const std::string& svcName, ServiceApi** apis) {
    skullmock_svc_t mockSvc;
    mockSvc.name = svcName.c_str();
    mockSvc.ud   = NULL;
    mockSvc.iocall  = _iocall;
    mockSvc.release = _release;

    skullut_module_mocksrv_add(this->utModule_, mockSvc);
    return true;
}

void UTModule::setTxnSharedData(const google::protobuf::Message& data) {
    // 1. prepare rawData
    TxnSharedRawData* rawData =
        (TxnSharedRawData*)skullut_module_data(this->utModule_);

    if (!rawData) {
        rawData = new TxnSharedRawData();
    }

    // 2. serialize msg to rawData
    void* idlData = NULL;
    int sz = data.ByteSize();
    if (sz) {
        idlData = calloc(1, (size_t)sz);
        bool r = data.SerializeToArray(idlData, sz);
        assert(r);
    }

    rawData->reset(idlData, (size_t)sz);

    // 3. store rawData
    skullut_module_setdata(this->utModule_, rawData);
}

google::protobuf::Message& UTModule::getTxnSharedData() {
    if (this->msg_) {
        delete this->msg_;
        this->msg_ = NULL;
    }

    const std::string& idlName = this->idlName_;
    const Descriptor* desc =
        DescriptorPool::generated_pool()->FindMessageTypeByName(idlName);
    const Message* new_msg =
        MessageFactory::generated_factory()->GetPrototype(desc);
    this->msg_ = new_msg->New();

    TxnSharedRawData* rawData =
        (TxnSharedRawData*)skullut_module_data(this->utModule_);

    if (rawData) {
        const void* binData = rawData->data();
        if (binData) {
            int size = (int)rawData->size();
            bool r = this->msg_->ParseFromArray(binData, size);
            assert(r);
        }
    }

    return *this->msg_;
}

const google::protobuf::Message& UTModule::getTxnSharedData() const {
    return getTxnSharedData();
}

/****************************** Service APIs **********************************/
UTService::UTService(const std::string& svcName, const std::string& config) {
    this->svcName_   = svcName;
    this->utService_ = skullut_service_create(svcName.c_str(), config.c_str(),
                                              "cpp", svc_getloader());
}

UTService::~UTService() {
    skullut_service_destroy(this->utService_);
}

class ServiceAPIData {
public:
    UTService& service_;
    const std::string& apiName_;
    google::protobuf::Message& uResp_;

public:
    ServiceAPIData(UTService& svc, const std::string& apiName,
        google::protobuf::Message& uResp)
        : service_(svc), apiName_(apiName), uResp_(uResp) {}

    ~ServiceAPIData() {}
};

static
void _api_validator(const void* req, size_t req_sz,
                    const void* resp, size_t resp_sz, void* ud) {
    ServiceAPIData* apiData = (ServiceAPIData*)ud;
    ServiceApiRespData apiResp(apiData->service_.svcName().c_str(),
                               apiData->apiName_.c_str(),
                               resp, resp_sz);

    apiData->uResp_.CopyFrom(apiResp.get());
}

void UTService::run(const std::string& apiName,
                    const google::protobuf::Message& req,
                    google::protobuf::Message& resp) {
    // 1. construct a service request
    ServiceApiReqData apiReq(this->svcName_.c_str(), apiName.c_str(), req, NULL);
    size_t dataSz = 0;
    ServiceApiReqRawData* rawReqData = apiReq.serialize(dataSz);

    // 2. construct api data
    ServiceAPIData apiData(*this, apiName, resp);

    // 3. run service api
    skullut_service_run(this->utService_, apiName.c_str(), rawReqData, dataSz,
                        _api_validator, &apiData);
}

const std::string& UTService::svcName() {
    return this->svcName_;
}

} // End of namespace
