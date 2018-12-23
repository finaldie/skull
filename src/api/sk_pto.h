#ifndef SK_PTO_H
#define SK_PTO_H

/**
 * This file is used for internal protocol. The goal is for this protocol:
 *  - Easy to pack and unpack
 *  - Use builtin types
 *  - Checksum support
 *
 * Some history: Before this version, we use protobuf-c as it, but finally found
 *  that the dependency management is a painful. And the performance is not as
 *  good as customized version, so no reason to keep it.
 *
 * Design details: Protocol contains runtime protocol and metadata. Runtime
 *  protocol is used for the inner communication, and the proto metadata contai-
 *  ns the control information which helps scheduler to route/callback/check the
 *  protocol data can be handled correctly.
 *
 *  - Protocol metadata {
 *      u32 id  :8;
 *      u32 pri :8;   // deliver to a specific type of IO
 *      u32 argc:16;
 *      u32 salt;     // For checksum
 *
 *      ops {
 *        .run
 *      };
 *  }
 *
 *  - Protocol Runtime = header + body
 *
 *    header: {
 *      u32 id  : 8,
 *      u32 chk : 8,  // = Checksum & 0xFF
 *      u32 argc: 16;
 *    }
 *
 *    body: {
 *      ..user defines..
 *    }
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
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

typedef union sk_arg_t {
    bool      b;
    int       i;
    uint32_t  u32;
    uint64_t  u64;
    int32_t   i32;
    int64_t   i64;
    void*     p;
    char*     s;
    char      c;
    void      (*f)();
    intptr_t  ip;
    uintptr_t uip;
} sk_arg_t;

/**
 * Protocol Header
 */
typedef struct sk_pto_hdr_t {
    uint32_t id  :8;
    uint32_t argc:8;
    uint32_t chk :16;  // = Checksum & 0xFF
} sk_pto_hdr_t;

typedef struct sk_proto_ops_t {
    int (*run) (const struct sk_sched_t* sched,
                const struct sk_sched_t* src,
                sk_entity_t* entity,
                sk_txn_t* txn,
                sk_pto_hdr_t* msg);
} sk_proto_ops_t;

/**
 * Protocol Metadata Structure
 */
typedef struct sk_proto_t {
    uint32_t id  :8;
    uint32_t pri :8;   // deliver to a specific type of IO
    uint32_t argc:8;
    uint32_t _padding :8;
    uint32_t salt;     // For checksum

    sk_proto_ops_t* ops;
} sk_proto_t;

// Protocol IDs
typedef enum sk_pto_id_t {
    SK_PTO_TCP_ACCEPT        = 0,
    SK_PTO_WORKFLOW_RUN,    // 1
    SK_PTO_ENTITY_DESTROY,  // 2
    SK_PTO_SVC_IOCALL,      // 3
    SK_PTO_SVC_TASK_RUN,    // 4
    SK_PTO_SVC_TASK_DONE,   // 5
    SK_PTO_TIMER_TRIGGERED, // 6
    SK_PTO_TIMER_EMIT,      // 7
    SK_PTO_STDIN_START,     // 8
    SK_PTO_SVC_TIMER_RUN,   // 9
    SK_PTO_SVC_TIMER_DONE,  // 10
    SK_PTO_SVC_TASK_CB,     // 11
    SK_PTO_MAX              // 12. Always the last one
} sk_pto_id_t;

/**
 * Protocol body
 */

// SK_PTO_TCP_ACCEPT
typedef struct sk_pto_tcp_accept_t {
    sk_arg_t fd;
} sk_pto_tcp_accept_t;

// SK_PTO_ENTITY_DESTROY
struct sk_pto_entity_destroy_t; // Empty proto

// SK_PTO_SVC_IOCALL
typedef struct sk_pto_service_iocall_t {
    sk_arg_t task_id;      // u64
    sk_arg_t service_name; // char*
    sk_arg_t api_name;     // char*
    sk_arg_t bio_idx;      // int
    sk_arg_t txn_task;     // void*
} sk_pto_service_iocall_t;

// SK_PTO_SVC_TASK_RUN
typedef struct sk_pto_service_task_run_t {
    sk_arg_t service_name; // char*

    sk_arg_t api_name;     // char*
    // 0: OK
    // 1: invalid service name
    // 2: service busy
    sk_arg_t io_status;    // int

    // caller scheduler
    sk_arg_t src;          // void*

    // txn taskdata
    sk_arg_t taskdata;     // void*
} sk_pto_service_task_run_t;

// SK_PTO_SVC_TASK_DONE
typedef struct sk_pto_service_task_done_t {
    sk_arg_t service_name;  // char*
    sk_arg_t resume_wf;     // bool
    sk_arg_t svc_task_done; // bool
} sk_pto_service_task_done_t;

// SK_PTO_SVC_TASK_CB
typedef struct sk_pto_service_task_cb_t {
    sk_arg_t taskdata;      // void*
    sk_arg_t service_name;  // char*
    sk_arg_t api_name;      // char*
    sk_arg_t task_status;   // int
    sk_arg_t svc_task_done; // bool
} sk_pto_service_task_cb_t;

// SK_PTO_SVC_TIMER_RUN
typedef struct sk_pto_service_timer_run_t {
    sk_arg_t svc;    // void*
    sk_arg_t job;    // func
    sk_arg_t ud;     // void*
    sk_arg_t valid;  // bool
    sk_arg_t status; // int
    sk_arg_t interval; // uint32
} sk_pto_service_timer_run_t;

// SK_PTO_SVC_TIMER_DONE
typedef struct sk_pto_service_timer_done_t {
    sk_arg_t svc;      // void*
    sk_arg_t status;   // int
    sk_arg_t interval; // uint32
} sk_pto_service_timer_done_t;

// SK_PTO_STDIN_START
struct sk_pto_stdin_start_t;

// SK_PTO_TIMER_EMIT
typedef struct sk_pto_timer_emit_t {
    sk_arg_t svc;   // void*
    sk_arg_t job;   // func
    sk_arg_t udata; // void*
    sk_arg_t valid; // bool
    sk_arg_t bidx;  // int
    sk_arg_t type;  // int
    sk_arg_t interval; // uint32
} sk_pto_timer_emit_t;

// SK_PTO_TIMER_TRIGGERED
typedef struct sk_pto_timer_triggered_t {
    sk_arg_t timer_obj; // void*
    sk_arg_t timer_cb;  // func
    sk_arg_t ud;        // void*
} sk_pto_timer_triggered_t;

// SK_PTO_WORKFLOW_RUN
struct sk_pto_workflow_run_t;

/******************************************************************************/
// Global protocol table
extern sk_proto_t sk_pto_tbl[];

// APIs
#define SK_PTO_BODYSZ(pto) (sizeof(sk_arg_t) * pto->argc)
#define SK_PTO_SZ(pto) (SK_PTO_BODYSZ(pto) + sizeof(sk_pto_hdr_t))

size_t    sk_pto_pack (uint32_t id, sk_pto_hdr_t*, va_list ap);
sk_arg_t* sk_pto_arg  (uint32_t id, sk_pto_hdr_t*, int idx);
bool      sk_pto_check(uint32_t id, sk_pto_hdr_t*);


#endif

