#include <stdlib.h>
#include <google/protobuf/descriptor.h>

#include "msg_factory.h"
#include "srv_utils.h"

namespace skullcpp {

using namespace google::protobuf;

ServiceApiReqData::ServiceApiReqData(skull_service_t* svc, const std::string& apiName) {
    this->svc_      = svc;
    this->svcName_  = std::string(skull_service_name(svc));
    this->apiName_  = apiName;
    this->descName_ = this->svcName_ + "-" + apiName + "_req.proto";
    this->msg_      = NULL;
    this->destroyMsg_ = true;

    size_t sz  = 0;
    void* data = skull_service_apidata(this->svc_, SKULL_API_REQ,  &sz);

    deserializeMsg(data, sz);

    (void)__padding;
    (void)__padding1;
}

ServiceApiReqData::ServiceApiReqData(const std::string& svcName,
                    const std::string& apiName,
                    const google::protobuf::Message& msg) {
    this->svc_      = NULL;
    this->svcName_  = svcName;
    this->apiName_  = apiName;
    this->descName_ = this->svcName_ + "-" + apiName + "_req.proto";
    this->msg_      = &(google::protobuf::Message&)msg;
    this->destroyMsg_ = false;

    (void)__padding;
    (void)__padding1;
}

ServiceApiReqData::ServiceApiReqData(const std::string& svcName,
                    const std::string& apiName,
                    const void* data, size_t sz) {
    this->svc_      = NULL;
    this->svcName_  = svcName;
    this->apiName_  = apiName;
    this->descName_ = this->svcName_ + "-" + apiName + "_req.proto";
    this->msg_      = NULL;
    this->destroyMsg_ = true;

    deserializeMsg(data, sz);

    (void)__padding;
    (void)__padding1;
}

ServiceApiReqData::ServiceApiReqData(const skullmock_task_t* task) {
    this->svc_      = NULL;
    this->svcName_  = std::string(task->service->name);
    this->apiName_  = std::string(task->api_name);
    this->descName_ = this->svcName_ + "-" + this->apiName_ + "_req.proto";
    this->msg_      = NULL;
    this->destroyMsg_ = true;

    size_t sz  = 0;
    void* data = skull_service_apidata(this->svc_, SKULL_API_REQ,  &sz);

    deserializeMsg(data, sz);

    (void)__padding;
    (void)__padding1;
}

ServiceApiReqData::~ServiceApiReqData() {
    if (!this->destroyMsg_) return;

    delete this->msg_;
}

void ServiceApiReqData::deserializeMsg(const void* data, size_t sz) {
    if (this->msg_) {
        return;
    }

    this->msg_ = MsgFactory::instance().newMsg(this->descName_);

    if (data && sz) {
        bool r = this->msg_->ParseFromArray(data, (int)sz);
        assert(r);
    }
}


const google::protobuf::Message& ServiceApiReqData::get() const {
    return *this->msg_;
}

void* ServiceApiReqData::serialize(size_t& sz) {
    size_t msg_sz = this->msg_->ByteSizeLong();

    void* data = NULL;
    if (msg_sz) {
        data = calloc(1, msg_sz);

        bool r = this->msg_->SerializeToArray(data, (int)msg_sz);
        assert(r);
    }

    sz = msg_sz;
    return data;
}

/********************************* Resp Data **********************************/
ServiceApiRespData::ServiceApiRespData(skull_service_t* svc,
                                       const std::string& apiName,
                                       bool storeBack) {
    this->svc_       = svc;
    this->descName_  = std::string(skull_service_name(svc)) + "-" + apiName + "_resp.proto";
    this->storeBack_ = storeBack;
    this->msg_       = NULL;

    size_t sz = 0;
    const void* data = skull_service_apidata(svc, SKULL_API_RESP, &sz);
    deserializeMsg(data, sz);

    (void)__padding;
    (void)__padding1;
}

ServiceApiRespData::ServiceApiRespData(const std::string& svcName,
                                       const std::string& apiName,
                                       const void* data, size_t sz) {
    this->svc_       = NULL;
    this->descName_  = std::string(svcName) + "-" + apiName + "_resp.proto";
    this->storeBack_ = false;
    this->msg_       = NULL;

    deserializeMsg(data, sz);

    (void)__padding;
    (void)__padding1;
}

ServiceApiRespData::ServiceApiRespData(skull_service_t* svc,
                                       const std::string& apiName,
                                       const void* data, size_t sz) {
    this->svc_       = svc;
    this->descName_  = std::string(skull_service_name(svc)) + "-" + apiName + "_resp.proto";
    this->storeBack_ = true;
    this->msg_       = NULL;

    deserializeMsg(data, sz);

    (void)__padding;
    (void)__padding1;
}

ServiceApiRespData::~ServiceApiRespData() {
    if (this->svc_ && this->storeBack_) {
        size_t sz = 0;
        void* data = skull_service_apidata(this->svc_, SKULL_API_RESP, &sz);

        if (data && sz) {
            free(data);
        }

        int newSz = (int)this->msg_->ByteSizeLong();
        if (newSz) {
            void* newData = calloc(1, (size_t)newSz);
            bool r = this->msg_->SerializeToArray(newData, newSz);
            assert(r);

            skull_service_apidata_set(this->svc_, SKULL_API_RESP,
                                      newData, (size_t)newSz);
        }
    }

    delete this->msg_;
}

void ServiceApiRespData::deserializeMsg(const void* data, size_t sz) {
    if (this->msg_) {
        return;
    }

    this->msg_ = MsgFactory::instance().newMsg(this->descName_);

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
