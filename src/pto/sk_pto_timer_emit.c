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
#include "api/sk_timer_service.h"

// This proto is ran in master thread
static
int _run (const sk_sched_t* sched, const sk_sched_t* src, sk_entity_t* entity,
          sk_txn_t* txn, sk_pto_hdr_t* msg)
{
    SK_ASSERT(entity);
    SK_ASSERT(SK_ENV_ENGINE == SK_ENV_CORE->master);
    SK_ASSERT(sk_pto_check(SK_PTO_TIMER_EMIT, msg));

    uint32_t id = SK_PTO_TIMER_EMIT;
    sk_service_t*  svc   = sk_pto_arg(id, msg, 0)->p;
    sk_service_job ujob  = sk_pto_arg(id, msg, 1)->f;
    sk_obj_t*      udata = sk_pto_arg(id, msg, 2)->p;
    int            valid = sk_pto_arg(id, msg, 3)->i;
    int            bidx  = sk_pto_arg(id, msg, 4)->i;
    sk_service_job_rw_t type = sk_pto_arg(id, msg, 5)->u32;
    uint32_t       interval  = sk_pto_arg(id, msg, 6)->u32;

    // 1. Create a service task (access -> write)
    sk_srv_task_t task;
    memset(&task, 0, sizeof(task));

    task.base.type = type == SK_SRV_JOB_READ
        ? SK_QUEUE_ELEM_READ : SK_QUEUE_ELEM_WRITE;
    task.type              = SK_SRV_TASK_TIMER;
    task.io_status         = SK_SRV_IO_STATUS_OK;
    task.bidx              = bidx;
    task.src               = src;
    task.data.timer.entity = entity;
    task.data.timer.job    = ujob;
    task.data.timer.ud     = udata;
    task.data.timer.valid  = valid;
    task.data.timer.interval = interval;

    // 2. Push to service task queue
    sk_print("push task to service queue\n");
    sk_srv_status_t ret = sk_service_push_task(svc, &task);
    if (ret != SK_SRV_STATUS_OK) {
        SK_ASSERT_MSG(ret == SK_SRV_STATUS_BUSY, "ret: %d\n", ret);
        task.io_status = SK_SRV_IO_STATUS_BUSY;

        SK_LOG_DEBUG(SK_ENV_LOGGER, "ServiceJob Busy, service: %s",
                     sk_service_name(svc));

        // Schedule it back to original caller
        sk_service_schedule_task(svc, &task);
        return 1;
    }

    // 3. Reschedule service tasks
    sk_service_schedule_tasks(svc);
    return 0;
}

sk_proto_ops_t sk_pto_ops_timer_emit = {
    .run = _run
};
