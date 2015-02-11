#ifndef SKULL_TXN_TYPES_H
#define SKULL_TXN_TYPES_H

#include <google/protobuf-c/protobuf-c.h>
#include "api/sk_txn.h"

struct _skull_txn_t {
    sk_txn_t* txn;

    // protobuf message related data
    ProtobufCMessage* idl;
    const ProtobufCMessageDescriptor* descriptor;
};

#endif

