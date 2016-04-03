#ifndef SKULLCPP_TXN_DATA_H
#define SKULLCPP_TXN_DATA_H

#include <stddef.h>
#include <skull/txndata.h>

namespace skullcpp {

class TxnData {
private:
    // Make noncopyable
    TxnData(const TxnData&);
    const TxnData& operator=(const TxnData&);

public:
    TxnData() {};
    virtual ~TxnData() {};

public:
    virtual void append(const void* data, size_t sz) = 0;
};

} // End of namespace

#endif

