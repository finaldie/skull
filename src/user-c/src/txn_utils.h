#ifndef SKULL_TXN_UTILS_H
#define SKULL_TXN_UTILS_H

#include <google/protobuf-c/protobuf-c.h>
#include "api/sk_txn.h"

struct _skull_txn_t {
    sk_txn_t* txn;

    // protobuf message related data
    ProtobufCMessage* idl;
    const ProtobufCMessageDescriptor* descriptor;
};

struct skull_txndata_t {
    sk_txn_t* txn;
};

/**
 * Unpack the txn sharedata and fill up the skull_txn
 */
void skull_txn_init(struct _skull_txn_t* skull_txn, const sk_txn_t*);

/**
 * Pack the txn sharedata and release unpacked resources
 */
void skull_txn_release(struct _skull_txn_t* skull_txn, sk_txn_t*);

#endif

