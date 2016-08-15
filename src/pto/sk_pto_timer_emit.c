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
int _run (sk_sched_t* sched, sk_sched_t* src, sk_entity_t* entity, sk_txn_t* txn,
          void* proto_msg)
{
    SK_ASSERT(entity);
    SK_ASSERT(SK_ENV_ENGINE == SK_ENV_CORE->master);

    TimerEmit* timer_emit_msg = proto_msg;
    sk_service_t*  svc   = (sk_service_t*) (uintptr_t) timer_emit_msg->svc;
    sk_obj_t*      udata = (void*) (uintptr_t) timer_emit_msg->udata;
    sk_service_job ujob  = *(sk_service_job*) timer_emit_msg->job.data;
    int            valid = timer_emit_msg->valid;
    int            bidx  = timer_emit_msg->bidx;
    sk_service_job_rw_t type = (sk_service_job_rw_t)timer_emit_msg->type;


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

    // 2. Push to service task queue
    sk_print("push task to service queue\n");
    sk_srv_status_t ret = sk_service_push_task(svc, &task);
    if (ret != SK_SRV_STATUS_OK) {
        SK_ASSERT_MSG(ret == SK_SRV_STATUS_BUSY, "ret: %d\n", ret);
        task.io_status = SK_SRV_IO_STATUS_BUSY;

        sk_metrics_worker.srv_timer_busy.inc(1);
        sk_metrics_global.srv_timer_busy.inc(1);

        SK_LOG_DEBUG(SK_ENV_LOGGER, "ServiceJob Busy, service: %s", sk_service_name(svc));

        // Schedule it back to original caller
        sk_service_schedule_task(svc, &task);
        return 1;
    }

    // 3. Reschedule service tasks
    sk_service_schedule_tasks(svc);
    return 0;
}

sk_proto_opt_t sk_pto_timer_emit = {
    .descriptor = &timer_emit__descriptor,
    .run = _run
};
