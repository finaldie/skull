#ifndef SK_TIMER_SERVICE_H
#define SK_TIMER_SERVICE_H

#include <stdint.h>
#include "api/sk_entity.h"

typedef struct sk_timersvc_t sk_timersvc_t;
typedef struct sk_timer_t sk_timer_t;
typedef void   (*sk_timer_triggered) (void* ud);

int sk_timer_valid(sk_timer_t*);
void sk_timer_cancel(sk_timer_t*);
sk_entity_t* sk_timer_entity(sk_timer_t*);
sk_timersvc_t* sk_timer_svc(sk_timer_t*);

sk_timersvc_t* sk_timersvc_create(void* evlp);
void sk_timersvc_destroy(sk_timersvc_t*);
uint32_t sk_timersvc_timeralive_cnt(sk_timersvc_t*);

sk_timer_t* sk_timersvc_timer_create(sk_timersvc_t*,
                                     sk_entity_t*,
                                     uint32_t expiration, // unit: millisecond
                                     sk_timer_triggered,
                                     void* ud);

// Destroy a timer when a timer has already been cancelled
void sk_timersvc_timer_destroy(sk_timersvc_t*, sk_timer_t*);

#endif

