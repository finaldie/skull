#ifndef SKULLCPP_IDL_INTERNAL_H
#define SKULLCPP_IDL_INTERNAL_H

#include <stddef.h>

namespace skullcpp {

class TxnSharedRawData {
private:
    void*  data_;
    size_t size_;

public:
    TxnSharedRawData() : data_(NULL), size_(0) {};
    TxnSharedRawData(const void* data, size_t sz);
    ~TxnSharedRawData();

public:
    const void* data() const;
    void* data();
    void  reset(const void* data, size_t sz);

    size_t size() const;
};

}

#endif

