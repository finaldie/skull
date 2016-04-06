#ifndef SKULLCPP_TXNDATA_IMP_H
#define SKULLCPP_TXNDATA_IMP_H

#include "skullcpp/txndata.h"

namespace skullcpp {

class TxnDataImp : public TxnData {
private:
    skull_txndata_t* txndata;

public:
    TxnDataImp(skull_txndata_t*);
    virtual ~TxnDataImp();

public:
    void append(const void* data, size_t sz);
};

} // End of namespace

#endif

