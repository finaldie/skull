#include <stdlib.h>
#include <google/protobuf/descriptor.h>

#include "srv_utils.h"

namespace skullcpp {

using namespace google::protobuf;

ServiceApi* findApi(ServiceApi** apis, const char* api_name) {
    if (!apis || !api_name) {
        return NULL;
    }

    for (int i = 0; apis[i] != NULL; i++) {
        ServiceApi* api = apis[i];

        if (0 == strcmp(api->name, api_name)) {
            return api;
        }
    }

    return NULL;
}

ServiceApiReqData::ServiceApiReqData(skull_service_t* svc, const char* apiName) {
    this->svc_      = svc;
    this->svcName_  = std::string(skull_service_name(svc));
    this->apiName_  = std::string(apiName);
    this->descName_ = this->svcName_ + "-" + apiName + "_req.proto";
    this->msg_      = NULL;
    this->destroyMsg_ = true;

    size_t sz  = 0;
    ServiceApiReqRawData* raw =
        (ServiceApiReqRawData*)skull_service_apidata(this->svc_, SKULL_API_REQ,  &sz);

    deserializeMsg(raw->data, raw->sz);
}

ServiceApiReqData::ServiceApiReqData(const ServiceApiReqRawData* rawData) {
    this->svc_ = NULL;
    this->svcName_ = rawData->svcName;
    this->apiName_ = rawData->apiName;
    this->descName_ = rawData->svcName + "-" + this->apiName_ + "_req.proto";
    this->msg_ = NULL;
    this->rawData_  = *rawData;
    this->destroyMsg_ = true;

    deserializeMsg(rawData->data, rawData->sz);
}

ServiceApiReqData::ServiceApiReqData(const char* svcName, const char* apiName,
                    const google::protobuf::Message& msg, Service::ApiCB cb) {
    this->svc_ = NULL;
    this->svcName_ = std::string(svcName);
    this->apiName_ = std::string(apiName);
    this->descName_ = this->svcName_ + "-" + apiName + "_req.proto";
    this->msg_ = &(google::protobuf::Message&)msg;
    this->rawData_.cb = cb;
    this->destroyMsg_ = false;
}

ServiceApiReqData::ServiceApiReqData(const skullmock_task_t* task) {
    this->svc_ = NULL;

    this->svcName_ = std::string(task->service->name);
    this->apiName_ = std::string(task->api_name);
    this->descName_ = this->svcName_ + "-" + this->apiName_ + "_req.proto";
    this->msg_ = NULL;
    this->destroyMsg_ = true;

    deserializeMsg(task->request, task->request_sz);
}

ServiceApiReqData::~ServiceApiReqData() {
    if (!this->destroyMsg_) return;

    delete this->msg_;
}

void ServiceApiReqData::deserializeMsg(const void* data, size_t sz) {
    if (this->msg_) {
        return;
    }

    const FileDescriptor* fileDesc =
        DescriptorPool::generated_pool()->FindFileByName(this->descName_);
    const Descriptor* desc = fileDesc->message_type(0);

    const Message* new_msg =
        MessageFactory::generated_factory()->GetPrototype(desc);

    this->msg_ = new_msg->New();

    if (data && sz) {
        bool r = this->msg_->ParseFromArray(data, (int)sz);
        assert(r);
    }
}


const google::protobuf::Message& ServiceApiReqData::get() const {
    return *this->msg_;
}

ServiceApiReqRawData* ServiceApiReqData::serialize(size_t& sz) {
    ServiceApiReqRawData* rawData = new ServiceApiReqRawData;
    rawData->sz = (size_t)this->msg_->ByteSize();
    rawData->data = calloc(1, rawData->sz);
    bool r = this->msg_->SerializeToArray(rawData->data, (int)rawData->sz);
    assert(r);

    rawData->svcName = this->svcName_;
    rawData->apiName = this->apiName_;
    rawData->cb = this->rawData_.cb;

    sz = sizeof(*rawData);
    return rawData;
}

/********************************* Resp Data **********************************/
ServiceApiRespData::ServiceApiRespData(skull_service_t* svc,
                                       const char* apiName,
                                       bool storeBack) {
    this->svc_       = svc;
    this->descName_  = std::string(skull_service_name(svc)) + "-" + apiName + "_resp.proto";
    this->storeBack_ = storeBack;
    this->msg_       = NULL;

    size_t sz = 0;
    const void* data = skull_service_apidata(svc, SKULL_API_RESP, &sz);
    deserializeMsg(data, sz);
}

ServiceApiRespData::ServiceApiRespData(const char* svcName,
                                       const char* apiName,
                                       const void* data, size_t sz) {
    this->svc_       = NULL;
    this->descName_  = std::string(svcName) + "-" + apiName + "_resp.proto";
    this->storeBack_ = false;
    this->msg_       = NULL;

    deserializeMsg(data, sz);
}

ServiceApiRespData::ServiceApiRespData(skull_service_t* svc,
                                       const char* apiName,
                                       const void* data, size_t sz) {
    this->svc_       = svc;
    this->descName_  = std::string(skull_service_name(svc)) + "-" + apiName + "_resp.proto";
    this->storeBack_ = true;
    this->msg_       = NULL;

    deserializeMsg(data, sz);
}

ServiceApiRespData::~ServiceApiRespData() {
    if (this->svc_ && this->storeBack_) {
        size_t sz = 0;
        void* data = skull_service_apidata(this->svc_, SKULL_API_RESP, &sz);

        if (data && sz) {
            free(data);
        }

        int newSz = this->msg_->ByteSize();
        void* newData = calloc(1, (size_t)newSz);
        bool r = this->msg_->SerializeToArray(newData, newSz);
        assert(r);

        skull_service_apidata_set(this->svc_, SKULL_API_RESP, newData, (size_t)newSz);
    }

    delete this->msg_;
}

void ServiceApiRespData::deserializeMsg(const void* data, size_t sz) {
    if (this->msg_) {
        return;
    }

    const FileDescriptor* fileDesc =
        DescriptorPool::generated_pool()->FindFileByName(this->descName_);
    const Descriptor* desc = fileDesc->message_type(0);

    const Message* new_msg =
        MessageFactory::generated_factory()->GetPrototype(desc);

    this->msg_ = new_msg->New();

    if (data && sz) {
        bool r = this->msg_->ParseFromArray(data, (int)sz);
        assert(r);
    }
}

const google::protobuf::Message& ServiceApiRespData::get() const {
    return *this->msg_;
}

google::protobuf::Message& ServiceApiRespData::get() {
    return *this->msg_;
}

} // End of namespace
