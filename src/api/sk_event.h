#ifndef SK_EVENT_H
#define SK_EVENT_H

#include "api/sk_types.h"

#define SK_EVENT_ST_RUNNING 0
#define SK_EVENT_ST_PAUSE   1
#define SK_EVENT_ST_STOP    2

#define SK_EVENT_SZ (sizeof(sk_event_t))

typedef struct sk_event_t {
    sk_ud_t data;
    int     pto_id;
    int     status;

    int  (*end) ();
    int  (*run) (sk_ud_t data);
    void (*cb)  (sk_ud_t data);
    void (*destroy) (sk_ud_t data);
} sk_event_t;

#endif

