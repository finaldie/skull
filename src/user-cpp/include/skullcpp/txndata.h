#ifndef SKULLCPP_TXN_DATA_H
#define SKULLCPP_TXN_DATA_H

#include <stddef.h>
#include <string>
#include <skull/txndata.h>

namespace skullcpp {

class TxnData {
private:
    // Make noncopyable
    TxnData(const TxnData&) = delete;
    TxnData(TxnData&&) = delete;
    TxnData& operator=(const TxnData&) = delete;
    TxnData& operator=(TxnData&&) = delete;

public:
    TxnData() {};
    virtual ~TxnData() {};

public:
    virtual void append(const void* data, size_t sz) = 0;
    virtual void append(const std::string& data) = 0;
};

} // End of namespace

#endif

