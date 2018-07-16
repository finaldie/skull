#include <stdlib.h>

#include "flibs/fev_timer_service.h"
#include "flibs/fhash.h"

#include "api/sk_utils.h"
#include "api/sk_env.h"
#include "api/sk_sched.h"
#include "api/sk_pto.h"
#include "api/sk_metrics.h"
#include "api/sk_timer_service.h"

#define TIMER_CHECK_DISABLED 0

struct sk_timersvc_t {
    fev_timer_svc* timer_service;

    uint32_t timer_alive;

    int      _padding;

    fhash*   timers;
};

struct sk_timer_t {
    sk_timersvc_t*     owner;
    sk_entity_t*       entity; // which entity belongs to
    ftimer_node*       timer;

    sk_timer_triggered trigger;
    sk_obj_t*          ud;

    uint32_t valid     :1;
    uint32_t triggered :1;
    uint32_t _reserved :30;

    int _padding;
};

sk_timersvc_t* sk_timersvc_create(void* evlp)
{
    sk_timersvc_t* svc = calloc(1, sizeof(*svc));
    svc->timer_service =
        fev_tmsvc_create(evlp, TIMER_CHECK_DISABLED, FEV_TMSVC_SINGLE_LINKED);
    svc->timers = fhash_u64_create(0, FHASH_MASK_AUTO_REHASH);

    SK_ASSERT(svc->timer_service);
    SK_ASSERT(svc->timers);
    return svc;
}

void sk_timersvc_destroy(sk_timersvc_t* svc)
{
    if (!svc) return;

    // 1. destroy un-triggered sk_timers
    fhash_u64_iter iter = fhash_u64_iter_new(svc->timers);
    sk_timer_t* timer = NULL;

    while ((timer = fhash_u64_next(&iter))) {
        sk_print("destroy un-triggered timer\n");
        sk_obj_destroy(timer->ud);
        free(timer);
    }

    fhash_u64_iter_release(&iter);
    fhash_u64_delete(svc->timers);

    // 2. destroy timer service
    fev_tmsvc_destroy(svc->timer_service);
    free(svc);
}

int  sk_timersvc_process(sk_timersvc_t* svc) {
    return fev_tmsvc_process(svc->timer_service);
}

sk_timer_t* sk_timersvc_first(sk_timersvc_t* svc) {
    ftimer_node* node = fev_tmsvc_first(svc->timer_service);
    return fev_tmsvc_timer_data(node);
}

long sk_timersvc_timer_expiration(sk_timer_t* timer) {
    return fev_tmsvc_timer_expiration(timer->timer);
}

uint32_t sk_timersvc_timeralive_cnt(sk_timersvc_t* svc)
{
    return svc->timer_alive;
}

static
void _timer_triggered(fev_state* state, void* arg)
{
    SK_ASSERT(arg);

    sk_timer_t* timer = arg;
    timer->triggered = 1;
    sk_print("tmsvc: timer triggered %p\n", (void*)timer);

    sk_sched_send(SK_ENV_SCHED, SK_ENV_SCHED, timer->entity, NULL, 0,
                  SK_PTO_TIMER_TRIGGERED, timer, timer->trigger, timer->ud);
}

sk_timer_t* sk_timersvc_timer_create(sk_timersvc_t* svc,
                                     sk_entity_t* entity,
                                     uint32_t expiration, // unit: millisecond
                                     sk_timer_triggered trigger,
                                     sk_obj_t* ud)
{
    SK_ASSERT(svc);
    SK_ASSERT(entity);
    SK_ASSERT(trigger);

    sk_timer_t* timer = calloc(1, sizeof(*timer));
    sk_print("tmsvc: create timer %p\n", (void*)timer);
    timer->owner  = svc;
    timer->entity = entity;
    timer->timer  = fev_tmsvc_timer_add(svc->timer_service, (long)expiration,
                                        _timer_triggered, timer);
    //sk_print("fev_timer %p\n", (void*) timer->timer);
    timer->trigger = trigger;
    timer->ud      = ud;
    timer->valid   = 1;

    svc->timer_alive++;

    SK_ASSERT(timer->timer);

    // Add it into timer holder
    fhash_u64_set(svc->timers, (uint64_t) (uintptr_t) timer, timer);

    // update metrics
    sk_metrics_global.timer_emit.inc(1);
    sk_metrics_worker.timer_emit.inc(1);
    return timer;
}

void sk_timersvc_timer_destroy(sk_timersvc_t* svc, sk_timer_t* timer)
{
    if (!timer) return;

    SK_ASSERT(svc);
    SK_ASSERT(svc == timer->owner);

    if (!timer->triggered) {
        fev_tmsvc_timer_del(timer->timer);
    }

    fhash_u64_del(svc->timers, (uint64_t) (uintptr_t) timer);
    sk_obj_destroy(timer->ud);

    free(timer);
    svc->timer_alive--;

    // update metrics
    sk_metrics_global.timer_complete.inc(1);
    sk_metrics_worker.timer_complete.inc(1);
}

int sk_timer_valid(sk_timer_t* timer)
{
    return timer && timer->valid;
}

void sk_timer_cancel(sk_timer_t* timer)
{
    if (!timer) return;
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

int sk_timer_reset(sk_timer_t* timer) {
    if (!timer) return 1;
    if (timer->triggered) return 1;

    int ret = fev_tmsvc_timer_reset(timer->timer);
    SK_ASSERT_MSG(!ret, "Reset timer failed\n");
    return 0;
}

int sk_timer_resetn(sk_timer_t* timer, uint32_t expiration) {
    if (!timer) return 1;
    if (timer->triggered) return 1;


    int ret = fev_tmsvc_timer_resetn(timer->timer, (long)expiration);
    SK_ASSERT_MSG(!ret, "Reset timer failed\n");
    return 0;
}
