#ifndef SKULLCPP_TXN_DATA_H
#define SKULLCPP_TXN_DATA_H

#include <stddef.h>
#include <skull/txndata.h>

namespace skullcpp {

class TxnData {
private:
    skull_txndata_t* txndata;

public:
    TxnData(skull_txndata_t*);
    ~TxnData();

public:
    void append(const void* data, size_t sz);
};

} // End of namespace

#endif

