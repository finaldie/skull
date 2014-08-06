#ifndef SK_PTO_H
#define SK_PTO_H

#include <stdint.h>
#include "api/sk_entity.h"

// Priority: 0 - 9
#define SK_PTO_PRI_0  0
#define SK_PTO_PRI_1  1
#define SK_PTO_PRI_2  2
#define SK_PTO_PRI_3  3
#define SK_PTO_PRI_4  4
#define SK_PTO_PRI_5  5
#define SK_PTO_PRI_6  6
#define SK_PTO_PRI_7  7
#define SK_PTO_PRI_8  8
#define SK_PTO_PRI_9  9
#define SK_PTO_PRI_MIN SK_PTO_PRI_0
#define SK_PTO_PRI_MAX SK_PTO_PRI_9
#define SK_PTO_PRI_SZ  (SK_PTO_PRI_MAX + 1)

// The gcc compiler rasie a warning but clang won't
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpadded"

struct sk_sched_t;
typedef struct sk_proto_t {
    int priority;   // deliver to a specific type of IO
    int padding;    // reserved

    const void* descriptor; // the message descriptor
    int (*run) (struct sk_sched_t* sched,
                sk_entity_t* entity,
                void* proto_msg);
} sk_proto_t;


// =================================================================
// Protocol 1, accept
#include "pto/idls/net_accept.pb-c.h"
#define SK_PTO_NET_ACCEPT 0
extern sk_proto_t sk_pto_net_accept;

// Protocol 2, net process
#include "pto/idls/net_proc.pb-c.h"
#define SK_PTO_NET_PROC   1
extern sk_proto_t sk_pto_net_proc;

// global protocol table
extern sk_proto_t* sk_pto_tbl[];

#pragma GCC diagnostic pop
#endif

