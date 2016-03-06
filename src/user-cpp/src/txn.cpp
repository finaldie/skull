#include <stdlib.h>
#include <string.h>
#include <google/protobuf/descriptor.h>

#include "skull/txn.h"

#include "txn_idldata.h"
#include "skullcpp/txn.h"

namespace skullcpp {

using namespace google::protobuf;

Txn::Txn(skull_txn_t* sk_txn) {
    this->txn_ = sk_txn;
    this->msg_ = NULL;
    this->destroyRawData_ = false;
}

Txn::Txn(skull_txn_t* sk_txn, bool destroyRawData) {
    this->txn_ = sk_txn;
    this->msg_ = NULL;
    this->destroyRawData_ = destroyRawData;
}

Txn::~Txn() {
    // 1. Serialize msg back to binary data
    TxnSharedRawData* rawData = (TxnSharedRawData*)skull_txn_data(this->txn_);

    if (!this->destroyRawData_) {
        void* idlData = NULL;
        int sz = this->msg_->ByteSize();
        if (sz) {
            idlData = calloc(1, (size_t)sz);
            bool r = this->msg_->SerializeToArray(idlData, sz);
            assert(r);
        }

        rawData->reset(idlData, (size_t)sz);
    } else {
        delete rawData;
        skull_txn_setdata(this->txn_, NULL);
    }

    // 2. clean up msg
    delete this->msg_;
}

google::protobuf::Message& Txn::data() {
    // 1. Create a new message according to idl name
    const char* idlName = skull_txn_idlname(this->txn_);
    std::string protoFileName = std::string(idlName) + ".proto";

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

Txn::Status Txn::status() {
    skull_txn_status_t st = skull_txn_status(this->txn_);

    if (st == SKULL_TXN_OK) {
        return Txn::TXN_OK;
    } else {
        return Txn::TXN_ERROR;
    }
}

skull_txn_t* Txn::txn() {
    return this->txn_;
}

} // End of namespace
