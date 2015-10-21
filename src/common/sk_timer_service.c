#include <stdlib.h>

#include "flibs/fev_timer_service.h"

#include "api/sk_utils.h"
#include "api/sk_env.h"
#include "api/sk_sched.h"
#include "api/sk_pto.h"
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
    sk_timersvc_t*     owner;
    sk_entity_t*       entity; // which entity belongs to
    ftimer_node*       timer;

    sk_timer_triggered trigger;
    void*              ud;

    uint32_t valid     :1;
    uint32_t triggered :1;
    uint32_t _reserved :30;

#if __WORDSIZE == 64
    int _padding;
#endif
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
    timer->triggered = 1;

    TimerTriggered timer_pto = TIMER_TRIGGERED__INIT;
    timer_pto.timer_obj.len  = sizeof (timer);
    timer_pto.timer_obj.data = (uint8_t*) timer;
    timer_pto.timer_cb.len   = sizeof ((uintptr_t) timer->trigger);
    timer_pto.timer_cb.data  = (uint8_t*) (uintptr_t) timer->trigger;
    timer_pto.ud.len         = sizeof (timer->ud);
    timer_pto.ud.data        = (uint8_t*) timer->ud;

    sk_sched_push(SK_ENV_SCHED, timer->entity, NULL, SK_PTO_TIMER_TRIGGERED,
                  &timer_pto);
}

sk_timer_t* sk_timersvc_timer_create(sk_timersvc_t* svc,
                                     sk_entity_t* entity,
                                     uint32_t expiration, // unit: millisecond
                                     sk_timer_triggered trigger,
                                     void* ud)
{
    SK_ASSERT(svc);
    SK_ASSERT(entity);
    SK_ASSERT(trigger);

    sk_timer_t* timer = calloc(1, sizeof(*timer));
    timer->owner = svc;
    timer->entity = entity;
    timer->timer = fev_tmsvc_add_timer(svc->timer_service, expiration,
                                       _timer_triggered, timer);
    timer->trigger = trigger;
    timer->ud = ud;
    timer->valid = 1;

    svc->timer_alive++;

    SK_ASSERT(timer->timer);
    return timer;
}

void sk_timersvc_timer_destroy(sk_timersvc_t* svc, sk_timer_t* timer)
{
    if (!timer) return;
    if (1 == timer->valid) return;

    SK_ASSERT(svc);
    SK_ASSERT(svc == timer->owner);

    fev_tmsvc_del_timer(timer->timer);
    free(timer);

    svc->timer_alive--;
}

int sk_timer_valid(sk_timer_t* timer)
{
    return timer->valid;
}

void sk_timer_cancel(sk_timer_t* timer)
{
    timer->valid = 0;
}

sk_entity_t* sk_timer_entity(sk_timer_t* timer)
{
    return timer->entity;
}

sk_timersvc_t* sk_timer_svc(sk_timer_t* timer)
{
    return timer->owner;
}
