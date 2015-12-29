#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "api/sk_utils.h"
#include "api/sk_metrics.h"
#include "api/sk_env.h"
#include "api/sk_sched.h"
#include "api/sk_object.h"
#include "api/sk_entity.h"
#include "api/sk_txn.h"
#include "api/sk_pto.h"
#include "api/sk_service.h"

// This proto is ran in the caller engine
static
int _run (sk_sched_t* sched, sk_sched_t* src,
          sk_entity_t* entity, sk_txn_t* txn,
          void* proto_msg)
{
    ServiceTimerRun* msg = proto_msg;
    sk_service_t*  service = (sk_service_t*) (uintptr_t) msg->svc;
    sk_service_job job     = *(sk_service_job*) msg->job.data;
    sk_obj_t*      ud      = (sk_obj_t*) (uintptr_t) msg->ud;
    int            valid   = msg->valid;

    job(service, ud, valid);
    sk_obj_destroy(ud);

    ServiceTimerComplete complete_msg = SERVICE_TIMER_COMPLETE__INIT;
    complete_msg.svc = (uintptr_t) (void*) service;
    sk_sched_send(sched, src, entity, NULL, SK_PTO_SVC_TIMER_COMPLETE,
                  &complete_msg, 0);

    return 0;
}

sk_proto_t sk_pto_svc_timer_run = {
    .priority = SK_PTO_PRI_9,
    .descriptor = &service_timer_run__descriptor,
    .run = _run
};