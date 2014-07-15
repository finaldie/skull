// sk_event is the internal protocol which used for commuicate with scheduler

#ifndef SK_EVENT_H
#define SK_EVENT_H

#include <stdint.h>
#include "api/sk_types.h"

#define SK_EV_ST_RUNNING 0
#define SK_EV_ST_PAUSE   1
#define SK_EV_ST_STOPPED 2

#define SK_EV_REQ  0
#define SK_EV_RESP 1

#define SK_EV_INCOMING 0
#define SK_EV_OUTGOING 1

#define SK_EVENT_SZ (sizeof(sk_event_t))

typedef struct sk_event_t {
    uint32_t deliver  :10;  // deliver to a specific type of IO
    uint32_t ev_type  :1;   // incoming or outgoing

    uint32_t type     :1;   // req or resp
    uint32_t reserved :20;
    uint32_t pto_id;

    sk_ud_t  data;
} sk_event_t;

struct sk_sched_t;
typedef struct sk_event_opt_t {
    int  (*req)     (struct sk_sched_t* sched, sk_event_t* event);
    int  (*resp)    (struct sk_sched_t* sched, sk_event_t* event);
    int  (*end)     (struct sk_sched_t* sched, sk_event_t* event);
    void (*destroy) (struct sk_sched_t* sched, sk_event_t* event);
} sk_event_opt_t;

// Protocol IDs
#define SK_PTO_NET_ACCEPT 0
#define SK_PTO_NET_PROC   1

// Protocol opt
extern sk_event_opt_t sk_pto_net_accept;
extern sk_event_opt_t sk_pto_net_proc;

// Protocol Table of worker sched
extern sk_event_opt_t* sk_event_tbl[];

#endif

