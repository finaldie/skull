#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "api/sk_utils.h"
#include "api/sk_env.h"
#include "api/sk_sched.h"
#include "api/sk_object.h"
#include "api/sk_entity.h"
#include "api/sk_txn.h"
#include "api/sk_pto.h"
#include "api/sk_timer_service.h"

static
int _run (sk_sched_t* sched, sk_entity_t* entity, sk_txn_t* txn,
          void* proto_msg)
{
    TimerTriggered* timer_pto = proto_msg;
    sk_timer_t* timer = (sk_timer_t*) (uintptr_t) timer_pto->timer_obj;
    sk_timersvc_t* timersvc = sk_timer_svc(timer);
    int timer_valid = sk_timer_valid(timer);

    sk_obj_t* ud = (sk_obj_t*) (uintptr_t) timer_pto->ud;
    sk_timer_triggered timer_cb =
        * (sk_timer_triggered*) timer_pto->timer_cb.data;

    // run timer callback
    sk_print("timer triggered proto: valid: %d\n", timer_valid);
    timer_cb(entity, timer_valid, ud);

    sk_timersvc_timer_destroy(timersvc, timer);
    return 0;
}

sk_proto_t sk_pto_timer_triggered = {
    .priority = SK_PTO_PRI_9,
    .descriptor = &timer_triggered__descriptor,
    .run = _run
};
