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

typedef struct timer_jobdata_t {
    sk_service_t*  service;
    sk_service_job job;
    sk_obj_t*      ud;
} timer_jobdata_t;

static
void _timer_jobdata_destroy(sk_ud_t ud)
{
    sk_print("destroy timer jobdata\n");
    timer_jobdata_t* jobdata = ud.ud;

    sk_obj_destroy(jobdata->ud);
    free(jobdata);
}

static
int _timerjob_create(sk_service_t* service,
                     sk_entity_t* entity,
                     sk_timer_triggered timer_cb,
                     uint32_t delayed,
                     sk_service_job job,
                     sk_obj_t* ud)
{
    SK_ASSERT(service);
    SK_ASSERT(entity);
    SK_ASSERT(timer_cb);
    SK_ASSERT(job);

    // Create sk_timer job callback data
    sk_print("create timer job\n");
    timer_jobdata_t* jobdata = calloc(1, sizeof(*jobdata));
    jobdata->service  = service;
    jobdata->job      = job;
    jobdata->ud       = ud;

    sk_ud_t cb_data  = {.ud = jobdata};
    sk_obj_opt_t opt = {.preset = NULL, .destroy = _timer_jobdata_destroy};

    sk_obj_t* param_obj = sk_obj_create(opt, cb_data);

    // Create sk_timer with callback data
    sk_timersvc_t* timersvc = SK_ENV_TMSVC;
    sk_timer_t* timer = sk_timersvc_timer_create(timersvc, entity, delayed,
                                                 timer_cb, param_obj);
    SK_ASSERT(timer);

    // Record metrics
    sk_metrics_global.srv_timer_emit.inc(1);
    sk_metrics_worker.srv_timer_emit.inc(1);

    return 0;
}

// This function must be run at master thread due to all the service management
// are all run at master
static
void _timer_triggered(sk_entity_t* entity, int valid, sk_obj_t* ud)
{
    // 1. Extract timer parameters
    sk_print("timer triggered\n");
    timer_jobdata_t* jobdata  = sk_obj_get(ud).ud;
    sk_service_t*    svc      = jobdata->service;
    sk_service_job   job      = jobdata->job;
    sk_obj_t*        udata    = jobdata->ud;

    sk_srv_task_t task;
    memset(&task, 0, sizeof(task));

    task.base.type          = SK_QUEUE_ELEM_WRITE;
    task.type               = SK_SRV_TASK_TIMER;
    task.io_status          = SK_SRV_IO_STATUS_OK;
    task.data.timer.service = svc;
    task.data.timer.job     = job;
    task.data.timer.ud      = udata;
    task.data.timer.valid   = valid;

    // 2. Push to service task queue
    sk_print("push task to service queue\n");
    sk_srv_status_t ret = sk_service_push_task(svc, &task);
    SK_ASSERT(ret == SK_SRV_STATUS_OK);

    // 3. Reschedule service tasks
    size_t cnt = sk_service_schedule_tasks(svc);
    printf("service %zu tasks\n", cnt);

    // 4. Recored metrics
    sk_metrics_global.srv_timer_complete.inc(1);
    sk_metrics_worker.srv_timer_complete.inc(1);
}

// This proto is ran in master thread
static
int _run (sk_sched_t* sched, sk_entity_t* entity, sk_txn_t* txn,
          void* proto_msg)
{
    TimerEmit* timer_emit_msg = proto_msg;
    sk_service_t*  svc      = (sk_service_t*) (uintptr_t) timer_emit_msg->svc;
    uint32_t       delayed  = timer_emit_msg->delayed;
    sk_obj_t*      udata    = (void*) (uintptr_t) timer_emit_msg->udata;
    sk_service_job ujob     = * (sk_service_job*) timer_emit_msg->job.data;

    sk_print("Create a timer\n");
    int ret = _timerjob_create(svc, entity, _timer_triggered,
                               delayed, ujob, udata);
    SK_ASSERT(!ret);
    return 0;
}

sk_proto_t sk_pto_timer_emit = {
    .priority = SK_PTO_PRI_9,
    .descriptor = &timer_emit__descriptor,
    .run = _run
};
