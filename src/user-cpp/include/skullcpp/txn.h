#ifndef SKULLCPP_TXN_H
#define SKULLCPP_TXN_H

#include <stddef.h>
#include <stdint.h>

#include <string>
#include <google/protobuf/message.h>

#include "skull/txn.h"

namespace skullcpp {

class Txn {
private:
    google::protobuf::Message* msg_;
    skull_txn_t* txn_;
    bool destroyRawData_;

#if __WORDSIZE == 64
    uint32_t __padding :24;
    uint32_t __padding1;
#else
    uint32_t __padding :24;
#endif

    typedef enum Status {
        TXN_OK = 0,
        TXN_ERROR
    } Status;

public:
    Txn(skull_txn_t*);
    Txn(skull_txn_t*, bool destroyRawData);
    ~Txn();

    google::protobuf::Message& data();
    Status status();

    skull_txn_t* txn();
};

} // End of namespace

#endif

