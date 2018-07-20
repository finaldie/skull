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
int _run (const sk_sched_t* sched, const sk_sched_t* src, sk_entity_t* entity,
          sk_txn_t* txn, sk_pto_hdr_t* msg)
{
    SK_ASSERT(sk_pto_check(SK_PTO_TIMER_TRIGGERED, msg));

    uint32_t id = SK_PTO_TIMER_TRIGGERED;
    sk_timer_t* timer = sk_pto_arg(id, msg, 0)->p;
    sk_timer_triggered timer_cb = sk_pto_arg(id, msg, 1)->f;
    sk_obj_t*   ud    = sk_pto_arg(id, msg, 2)->p;

    sk_timersvc_t* timersvc = sk_timer_svc(timer);
    int timer_valid = sk_timer_valid(timer);

    // run timer callback
    sk_print("timer triggered proto: valid: %d\n", timer_valid);
    timer_cb(entity, timer_valid, ud);

    sk_timersvc_timer_destroy(timersvc, timer);
    return 0;
}

sk_proto_ops_t sk_pto_ops_timer_triggered = {
    .run = _run
};
