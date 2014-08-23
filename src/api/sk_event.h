// sk_event is the internal protocol which used for commuicate with scheduler

#ifndef SK_EVENT_H
#define SK_EVENT_H

#include <stdint.h>
#include <stddef.h>

#define SK_EVENT_SZ (sizeof(sk_event_t))

typedef struct sk_event_t {
    uint32_t pto_id;
    int      _reserved;
    size_t   sz;            // data size

    void*    txn;           // transcation data
    void*    data;          // protobuf data
} sk_event_t;

#endif

