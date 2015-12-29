// sk_event is the internal protocol which used for commuicate with scheduler

#ifndef SK_EVENT_H
#define SK_EVENT_H

#include <stdint.h>
#include <stddef.h>

#define SK_EVENT_SZ (sizeof(sk_event_t))

struct sk_sched_t;

typedef struct sk_event_t {
    struct sk_sched_t* src;
    struct sk_sched_t* dst;

    uint32_t pto_id;
    uint32_t hop    :4;
    uint32_t flags  :28;

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

