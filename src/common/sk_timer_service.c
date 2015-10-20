#include <stdlib.h>

#include "flibs/fev_timer_service.h"

#include "api/sk_utils.h"
#include "api/sk_timer_service.h"

#define TIMER_CHECK_INTERVAL 1

struct sk_timersvc_t {
    fev_timer_svc* timer_service;

    uint32_t timer_alive;

#if __WORDSIZE == 64
    int      _padding;
#endif
};

struct sk_timer_t {
    sk_timersvc_t*   owner;
    ftimer_node*     timer;

    sk_timer_trigger trigger;
    void*            ud;
};

sk_timersvc_t* sk_timersvc_create(void* evlp)
{
    sk_timersvc_t* svc = calloc(1, sizeof(*svc));
    svc->timer_service = fev_create_timer_service(
                            evlp, TIMER_CHECK_INTERVAL,
                            FEV_TMSVC_SINGLE_LINKED);

    SK_ASSERT(svc->timer_service);
    return svc;
}

void sk_timersvc_destroy(sk_timersvc_t* svc)
{
    if (!svc) return;

    fev_delete_timer_service(svc->timer_service);
    free(svc);
}

uint32_t sk_timersvc_timeralive_cnt(sk_timersvc_t* svc)
{
    return svc->timer_alive;
}

static
void _timer_triggered(fev_state* state, void* arg)
{
    sk_timer_t* timer = arg;
    timer->trigger(timer->ud);
}

sk_timer_t* sk_timersvc_timer_create(sk_timersvc_t* svc,
                                     uint32_t expiration, // unit: millisecond
                                     sk_timer_trigger trigger,
                                     void* ud)
{
    SK_ASSERT(svc);
    SK_ASSERT(trigger);

    sk_timer_t* timer = calloc(1, sizeof(*timer));
    timer->owner = svc;
    timer->timer = fev_tmsvc_add_timer(svc->timer_service, expiration,
                                       _timer_triggered, timer);
    timer->trigger = trigger;
    timer->ud = ud;

    svc->timer_alive++;

    SK_ASSERT(timer->timer);
    return timer;
}

void sk_timersvc_timer_destroy(sk_timersvc_t* svc, sk_timer_t* timer)
{
    if (!timer) return;

    SK_ASSERT(svc);
    SK_ASSERT(svc == timer->owner);

    fev_tmsvc_del_timer(timer->timer);
    free(timer);

    svc->timer_alive--;
}

