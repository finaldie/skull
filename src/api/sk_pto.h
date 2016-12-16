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

struct sk_sched_t;

typedef struct sk_proto_opt_t {
    const void* descriptor; // the message descriptor

    int (*run) (const struct sk_sched_t* sched,
                const struct sk_sched_t* src,
                sk_entity_t* entity,
                sk_txn_t* txn,
                void* proto_msg);
} sk_proto_opt_t;

typedef struct sk_proto_t {
    int pto_id;
    int priority;   // deliver to a specific type of IO

    sk_proto_opt_t* opt;
} sk_proto_t;

// =================================================================
// The gcc compiler rasie a warning but clang won't
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpadded"

#include "pto/idls/net_accept.pb-c.h"
#include "pto/idls/workflow_run.pb-c.h"
#include "pto/idls/entity_destroy.pb-c.h"
#include "pto/idls/service_iocall.pb-c.h"
#include "pto/idls/service_task.pb-c.h"
#include "pto/idls/timer_triggered.pb-c.h"
#include "pto/idls/timer_emit.pb-c.h"
#include "pto/idls/stdin_start.pb-c.h"
#include "pto/idls/service_timer.pb-c.h"

#pragma GCC diagnostic pop

// Protocol IDs
#define SK_PTO_NET_ACCEPT               0
#define SK_PTO_WORKFLOW_RUN             1
#define SK_PTO_ENTITY_DESTROY           2
#define SK_PTO_SVC_IOCALL               3
#define SK_PTO_SVC_TASK_RUN             4
#define SK_PTO_SVC_TASK_COMPLETE        5
#define SK_PTO_TIMER_TRIGGERED          6
#define SK_PTO_TIMER_EMIT               7
#define SK_PTO_STDIN_START              8
#define SK_PTO_SVC_TIMER_RUN            9
#define SK_PTO_SVC_TIMER_COMPLETE       10
#define SK_PTO_SVC_TASK_CB              11

extern sk_proto_opt_t sk_pto_net_accept;
extern sk_proto_opt_t sk_pto_workflow_run;
extern sk_proto_opt_t sk_pto_entity_destroy;
extern sk_proto_opt_t sk_pto_srv_iocall;
extern sk_proto_opt_t sk_pto_srv_task_run;
extern sk_proto_opt_t sk_pto_srv_task_complete;
extern sk_proto_opt_t sk_pto_timer_triggered;
extern sk_proto_opt_t sk_pto_timer_emit;
extern sk_proto_opt_t sk_pto_stdin_start;
extern sk_proto_opt_t sk_pto_svc_timer_run;
extern sk_proto_opt_t sk_pto_svc_timer_complete;
extern sk_proto_opt_t sk_pto_srv_task_cb;

/******************************************************************************/
// global protocol table
extern sk_proto_t sk_pto_tbl[];

#endif

