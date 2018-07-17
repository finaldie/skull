// sk_event is the internal protocol which used for commuicate with scheduler

#ifndef SK_EVENT_H
#define SK_EVENT_H

#include <stdint.h>
#include <stddef.h>

#define SK_EVENT_SZ (sizeof(sk_event_t))

struct sk_sched_t;

typedef struct sk_event_t {
    const struct sk_sched_t* src;
    const struct sk_sched_t* dst;

    uint32_t pto_id;
    uint32_t hop    :8;
    uint32_t flags  :24;

    // Data size
    size_t   sz;

    // Entity object, some major protos use it, a field for it here because it
    //  would improve the performance
    void*    entity;

    // Transaction data, some major protos use it, a field for it here because
    //  it would improve the performance
    void*    txn;

    // Protobuf data
    void*    data;
} sk_event_t;

#endif

