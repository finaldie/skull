#include <stdlib.h>
#include <string.h>
#include <google/protobuf/descriptor.h>

#include "skull/txn.h"

#include "txn_idldata.h"
#include "srv_utils.h"
#include "txn_imp.h"
#include "skullcpp/txn.h"

namespace skullcpp {

using namespace google::protobuf;

TxnImp::TxnImp(skull_txn_t* sk_txn) {
    this->txn_ = sk_txn;
    this->msg_ = NULL;
    this->destroyRawData_ = false;

#if __WORDSIZE == 64
    (void)__padding;
    (void)__padding1;
#else
    (void)__padding;
#endif
}

TxnImp::TxnImp(skull_txn_t* sk_txn, bool destroyRawData) {
    this->txn_ = sk_txn;
    this->msg_ = NULL;
    this->destroyRawData_ = destroyRawData;
}

TxnImp::~TxnImp() {
    // 1. Serialize msg back to binary data
    TxnSharedRawData* rawData = (TxnSharedRawData*)skull_txn_data(this->txn_);

    if (!this->destroyRawData_) {
        // Fill back the data when msg_ has be used before
        if (this->msg_) {
            void* idlData = NULL;
            int sz = this->msg_->ByteSize();
            if (sz) {
                idlData = calloc(1, (size_t)sz);
                bool r = this->msg_->SerializeToArray(idlData, sz);
                assert(r);
            }

            rawData->reset(idlData, (size_t)sz);
        }
    } else {
        delete rawData;
        skull_txn_setdata(this->txn_, NULL);
    }

    // 2. clean up msg
    delete this->msg_;
}

google::protobuf::Message& TxnImp::data() {
    // 1. Create a new message according to idl name
    const char* idlName = skull_txn_idlname(this->txn_);
    std::string protoFileName = std::string(idlName) + ".proto";

    // 1.1 Find the top 1 Message Descriptor
    const FileDescriptor* fileDesc =
        DescriptorPool::generated_pool()->FindFileByName(protoFileName);
    const Descriptor* desc = fileDesc->message_type(0);

    const Message* new_msg =
        MessageFactory::generated_factory()->GetPrototype(desc);

    this->msg_ = new_msg->New();

    // 2. Create TxnSharedRawData if needed
    TxnSharedRawData* rawData = (TxnSharedRawData*)skull_txn_data(this->txn_);
    if (!rawData) {
        rawData = new TxnSharedRawData();
        skull_txn_setdata(this->txn_, rawData);
    }

    // 3. Restore txn shared data if possible
    const void* binData = rawData->data();
    if (binData) {
        int size = (int)rawData->size();
        bool r = this->msg_->ParseFromArray(binData, size);
        assert(r);
    }

    return *this->msg_;
}

Txn::Status TxnImp::status() const {
    skull_txn_status_t st = skull_txn_status(this->txn_);

    if (st == SKULL_TXN_OK) {
        return Txn::TXN_OK;
    } else {
        return Txn::TXN_ERROR;
    }
}

static
int _skull_svc_api_callback(skull_txn_t* sk_txn, const char* apiName,
                            const void* request, size_t req_sz,
                            const void* response, size_t resp_sz) {
    TxnImp txn(sk_txn);
    const ServiceApiReqRawData* rawData = (const ServiceApiReqRawData*)request;
    ServiceApiReqData apiReq(rawData);
    ServiceApiRespData apiResp(rawData->svcName, apiName, response, resp_sz);

    return rawData->cb(txn, std::string(apiName), apiReq.get(), apiResp.get());
}

Txn::IOStatus TxnImp::serviceCall (const std::string& serviceName,
                                const std::string& apiName,
                                const google::protobuf::Message& request,
                                int bioIdx,
                                ApiCB cb) {
    // 1. Construct raw req data
    ServiceApiReqData apiReq(serviceName, apiName, request, cb);
    size_t dataSz = 0;
    ServiceApiReqRawData* rawReqData = apiReq.serialize(dataSz);

    // 2. Send service call to core
    skull_service_ret_t ret =
        skull_service_async_call(this->txn(), serviceName.c_str(),
            apiName.c_str(), rawReqData, dataSz,
            _skull_svc_api_callback, bioIdx);

    if (ret != SKULL_SERVICE_OK) {
        delete rawReqData;
    }

    // 3. Return status code
    switch (ret) {
    case SKULL_SERVICE_OK:
        return Txn::IOStatus::OK;
    case SKULL_SERVICE_ERROR_SRVNAME:
        return Txn::IOStatus::ERROR_SRVNAME;
    case SKULL_SERVICE_ERROR_APINAME:
        return Txn::IOStatus::ERROR_APINAME;
    case SKULL_SERVICE_ERROR_BIO:
        return Txn::IOStatus::ERROR_BIO;
    default:
        assert(0);
        return Txn::IOStatus::OK;
    }
}

Txn::IOStatus TxnImp::serviceCall (const std::string& serviceName,
                                const std::string& apiName,
                                const google::protobuf::Message& request,
                                ApiCB cb) {
    return serviceCall(serviceName, apiName, request, 0, cb);
}

skull_txn_t* TxnImp::txn() const {
    return this->txn_;
}

} // End of namespace
