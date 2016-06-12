#include "skullcpp/txndata.h"
#include "txndata_imp.h"

namespace skullcpp {

TxnDataImp::TxnDataImp(skull_txndata_t* txndata) {
    this->txndata = txndata;
}

TxnDataImp::~TxnDataImp() {
}

void TxnDataImp::append(const void* data, size_t sz) {
    skull_txndata_output_append(this->txndata, data, sz);
}

void TxnDataImp::append(const std::string& data) {
    skull_txndata_output_append(this->txndata, data.c_str(), data.length());
}

} // End of namespace
