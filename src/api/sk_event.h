// sk_event is the internal protocol which used for commuicate with scheduler

#ifndef SK_EVENT_H
#define SK_EVENT_H

#include <stdint.h>
#include "api/sk_types.h"

#define SK_EVENT_ST_RUNNING 0
#define SK_EVENT_ST_PAUSE   1
#define SK_EVENT_ST_STOPPED 2

#define SK_EVENT_SZ (sizeof(sk_event_t))

#define SK_PTO_REQ  0
#define SK_PTO_RESP 1

struct sk_sched_t;
typedef struct sk_event_opt_t {
    int  (*req) (void* evlp, struct sk_sched_t* sched, sk_ud_t data);
    int  (*resp) (void* evlp, struct sk_sched_t* sched, sk_ud_t data);
    int  (*end) (void* evlp, struct sk_sched_t* sched, sk_ud_t data);
    void (*destroy) (void* evlp, struct sk_sched_t* sched, sk_ud_t data);
} sk_event_opt_t;

typedef struct sk_event_t {
    uint32_t pto_id;
    uint32_t type     :1;   // req or resp
    uint32_t error    :1;
    uint32_t deliver  :16;
    uint32_t reserved :14;
    sk_ud_t  data;
} sk_event_t;

void sk_event_create(uint32_t pto_id, uint32_t deliver_to, sk_ud_t ud,
                     sk_event_t* event/*out*/);
void sk_event_run(sk_event_t* event);

#endif

