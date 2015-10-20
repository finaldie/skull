#ifndef SK_PTO_H
#define SK_PTO_H

#include "api/sk_entity.h"
#include "api/sk_txn.h"

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
                sk_txn_t* txn,
                void* proto_msg);
} sk_proto_t;


// =================================================================
// Protocol 1, accept
#include "pto/idls/net_accept.pb-c.h"
#define SK_PTO_NET_ACCEPT 0
extern sk_proto_t sk_pto_net_accept;

// Protocol 2, run the workflow
#include "pto/idls/workflow_run.pb-c.h"
#define SK_PTO_WORKFLOW_RUN   1
extern sk_proto_t sk_pto_workflow_run;

// Protocol 3, net process
#include "pto/idls/net_destroy.pb-c.h"
#define SK_PTO_NET_DESTROY   2
extern sk_proto_t sk_pto_net_destroy;

// Protocol 4, service io call
#include "pto/idls/service_iocall.pb-c.h"
#define SK_PTO_SERVICE_IOCALL 3
extern sk_proto_t sk_pto_srv_iocall;

// Protocol 5, run a service task
#include "pto/idls/service_task.pb-c.h"
#define SK_PTO_SERVICE_TASK_RUN 4
extern sk_proto_t sk_pto_srv_task_run;

// Protocol 6, service task complete notification
#define SK_PTO_SERVICE_TASK_COMPLETE 5
extern sk_proto_t sk_pto_srv_task_complete;

// global protocol table
extern sk_proto_t* sk_pto_tbl[];

#pragma GCC diagnostic pop
#endif

