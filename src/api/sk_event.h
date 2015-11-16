// sk_event is the internal protocol which used for commuicate with scheduler

#ifndef SK_EVENT_H
#define SK_EVENT_H

#include <stdint.h>
#include <stddef.h>

#define SK_EVENT_SZ (sizeof(sk_event_t))

typedef struct sk_event_t {
    uint32_t pto_id;
    int      _reserved;

    // data size
    size_t   sz;

    // entity object, some major protos use it, a field for it here because it
    // would improve the performance
    void*    entity;

    // transcation data, some major protos use it, a field for it here because
    // it would improve the performance
    void*    txn;

    // protobuf data
    void*    data;
} sk_event_t;

#endif

