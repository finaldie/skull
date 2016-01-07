#ifndef SKULL_SERVICE_TYPES_H
#define SKULL_SERVICE_TYPES_H

#include "api/sk_txn.h"
#include "api/sk_service.h"

struct _skull_service_t {
    sk_service_t*      service;
    const sk_txn_t*    txn;
    sk_txn_taskdata_t* task;
};

#endif

