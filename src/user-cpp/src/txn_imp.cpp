#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <google/protobuf/descriptor.h>

#include "skull/txn.h"
#include "skullcpp/txn.h"

#include "txn_idldata.h"
#include "srv_utils.h"
#include "msg_factory.h"
#include "txn_imp.h"

namespace skullcpp {

using namespace google::protobuf;

/**
 * gcc 4.6.x doesn't support calling another constructor in the same class
 *  upgrade to gcc 4.7 would solve this issue.
 *
 * So currently use another `init()` instead.
 */
void TxnImp::init(skull_txn_t* sk_txn, bool destroyRawData) {
    this->txn_ = sk_txn;
    this->msg_ = NULL;
    this->destroyRawData_ = destroyRawData;

    skull_txn_peer_t peer;
    skull_txn_peer(sk_txn, &peer);
    this->peerName_ = peer.name;
    this->peerPort_ = peer.port;

    this->peerType_ = (PeerType)skull_txn_peertype(sk_txn);
}

TxnImp::TxnImp(skull_txn_t* sk_txn) {
    init(sk_txn, false);

#if __WORDSIZE == 64
    (void)__padding;
    (void)__padding1;
#else
    (void)__padding;
#endif
}

TxnImp::TxnImp(skull_txn_t* sk_txn, bool destroyRawData) {
    init(sk_txn, destroyRawData);
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

    this->msg_ = MsgFactory::instance().newMsg(protoFileName);

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

    if (st == SKULL_TXN_ERROR) {
        return Txn::TXN_ERROR;
    } else if (st == SKULL_TXN_TIMEOUT) {
        return Txn::TXN_TIMEOUT;
    } else {
        return Txn::TXN_OK;
    }
}

const std::string& TxnImp::peerName() const {
    return this->peerName_;
}

int TxnImp::peerPort() const {
    return this->peerPort_;
}

Txn::PeerType TxnImp::peerType() const {
    return this->peerType_;
}

static
int _skull_svc_api_callback(skull_txn_t* sk_txn, skull_txn_ioret_t ret,
                            const char* serviceName,
                            const char* apiName,
                            const void* request, size_t reqSz,
                            const void* response, size_t respSz, void* ud) {
    TxnImp txn(sk_txn);
    ServiceApiReqData apiReq(serviceName, apiName, request, reqSz);
    ServiceApiRespData apiResp(serviceName, apiName, response, respSz);

    Txn::IOStatus st;
    if (ret == SKULL_TXN_IO_OK) {
        st = Txn::IOStatus::OK;
    } else if (ret == SKULL_TXN_IO_ERROR_SRVBUSY) {
        st = Txn::IOStatus::ERROR_SRVBUSY;
    } else {
        assert(0);
    }

    Txn::ApiCB cb = (Txn::ApiCB)(uintptr_t)ud;
    int r = cb(txn, st, std::string(apiName), apiReq.get(), apiResp.get());

    // Release the request data
    free((void*)request);
    return r;
}

Txn::IOStatus TxnImp::iocall (const std::string& serviceName,
                              const std::string& apiName,
                              const google::protobuf::Message& request,
                              int bioIdx,
                              ApiCB cb) {
    // 1. Construct raw req data
    ServiceApiReqData apiReq(serviceName, apiName, request);
    size_t dataSz = 0;
    void* rawReqData = apiReq.serialize(dataSz);

    void* void_cb = (void*)(uintptr_t)cb;

    // 2. Send service call to core
    skull_txn_ioret_t ret = skull_txn_iocall(this->txn(), serviceName.c_str(),
            apiName.c_str(), rawReqData, dataSz,
            cb ? _skull_svc_api_callback : NULL, bioIdx, void_cb);

    if (ret != SKULL_TXN_IO_OK) {
        free(rawReqData);
    }

    // 3. Return status code
    switch (ret) {
    case SKULL_TXN_IO_OK:
        return Txn::IOStatus::OK;
    case SKULL_TXN_IO_ERROR_SRVNAME:
        return Txn::IOStatus::ERROR_SRVNAME;
    case SKULL_TXN_IO_ERROR_APINAME:
        return Txn::IOStatus::ERROR_APINAME;
    case SKULL_TXN_IO_ERROR_STATE:
        return Txn::IOStatus::ERROR_STATE;
    case SKULL_TXN_IO_ERROR_BIO:
        return Txn::IOStatus::ERROR_BIO;
    default:
        assert(0);
        return Txn::IOStatus::OK;
    }
}

Txn::IOStatus TxnImp::iocall (const std::string& serviceName,
                              const std::string& apiName,
                              const google::protobuf::Message& request,
                              ApiCB cb) {
    return iocall(serviceName, apiName, request, 0, cb);
}

Txn::IOStatus TxnImp::iocall (const std::string& serviceName,
                              const std::string& apiName,
                              const google::protobuf::Message& request) {
    return iocall(serviceName, apiName, request, 0, NULL);
}

skull_txn_t* TxnImp::txn() const {
    return this->txn_;
}

} // End of namespace
