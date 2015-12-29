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
    sk_service_t*  svc      = (sk_service_t*) (uintptr_t) timer_emit_msg->svc;
    sk_obj_t*      udata    = (void*) (uintptr_t) timer_emit_msg->udata;
    sk_service_job ujob     = *(sk_service_job*) timer_emit_msg->job.data;
    int            valid    = timer_emit_msg->valid;

    // 1. Create a service task (access -> write)
    sk_srv_task_t task;
    memset(&task, 0, sizeof(task));

    task.base.type          = SK_QUEUE_ELEM_WRITE;
    task.type               = SK_SRV_TASK_TIMER;
    task.io_status          = SK_SRV_IO_STATUS_OK;
    task.src                = src;
    task.data.timer.entity  = entity;
    task.data.timer.job     = ujob;
    task.data.timer.ud      = udata;
    task.data.timer.valid   = valid;

    // 2. Push to service task queue
    sk_print("push task to service queue\n");
    sk_srv_status_t ret = sk_service_push_task(svc, &task);
    SK_ASSERT(ret == SK_SRV_STATUS_OK);

    // 3. Reschedule service tasks
    size_t cnt = sk_service_schedule_tasks(svc);
    printf("service %zu tasks\n", cnt);

    return 0;
}

sk_proto_t sk_pto_timer_emit = {
    .priority = SK_PTO_PRI_9,
    .descriptor = &timer_emit__descriptor,
    .run = _run
};
