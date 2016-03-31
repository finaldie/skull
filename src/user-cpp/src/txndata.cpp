#include "skullcpp/txndata.h"

namespace skullcpp {

TxnData::TxnData(skull_txndata_t* txndata) {
    this->txndata = txndata;
}

TxnData::~TxnData() {
}

void TxnData::append(const void* data, size_t sz) {
    skull_txndata_output_append(this->txndata, data, sz);
}

} // End of namespace
