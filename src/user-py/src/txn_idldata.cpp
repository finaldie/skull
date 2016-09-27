#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "txn_idldata.h"

namespace skullpy {

TxnSharedRawData::TxnSharedRawData(const void* data, size_t sz) {
    this->data_ = (void*)data;
    this->size_ = sz;
}

TxnSharedRawData::~TxnSharedRawData() {
    free(this->data_);
    this->size_ = 0;
}

const void* TxnSharedRawData::data() const {
    return this->data_;
}

void* TxnSharedRawData::data() {
    return this->data_;
}

size_t TxnSharedRawData::size() const {
    return this->size_;
}

void TxnSharedRawData::reset(const void* data, size_t sz) {
    if (this->data_ == data) {
        this->size_ = sz;
        return;
    }

    if (!data) {
        assert(sz == 0);
        free(this->data_);
        this->size_ = sz;
    } else {
        assert(sz > 0);
        free(this->data_);

        this->data_ = (void*)data;
        this->size_ = sz;
    }
}

} // End of namespace
