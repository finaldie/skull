#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "api/sk_utils.h"
#include "api/sk_env.h"
#include "api/sk_sched.h"
#include "api/sk_entity.h"
#include "api/sk_txn.h"
#include "api/sk_pto.h"
#include "api/sk_service.h"
#include "api/sk_timer_service.h"

typedef struct timer_jobdata_t {
    sk_service_t*  service;
    sk_service_job job;
    void*          ud;
    uint32_t       interval;

#if __WORDSIZE == 64
    int            _padding;
#endif
} timer_jobdata_t;

static
int _timerjob_create(sk_service_t* service,
                     sk_entity_t* entity,
                     sk_timer_triggered timer_cb,
                     uint32_t delayed,
                     uint32_t interval,
                     sk_service_job job,
                     void* ud)
{
    SK_ASSERT(service);
    SK_ASSERT(entity);
    SK_ASSERT(timer_cb);
    SK_ASSERT(job);

    timer_jobdata_t* jobdata = calloc(1, sizeof(*jobdata));
    jobdata->service  = service;
    jobdata->job      = job;
    jobdata->ud       = ud;
    jobdata->interval = interval;

    sk_timersvc_t* timersvc = SK_ENV_TMSVC;
    sk_timer_t* timer = sk_timersvc_timer_create(timersvc, entity, delayed,
                                                 timer_cb, jobdata);
    SK_ASSERT(timer);
    return 0;
}

// This function must be run at master thread due to all the service management
// are all run at master
static
void _timer_triggered(sk_entity_t* entity, int valid, void* ud)
{
    // 1. Rush a service task
    timer_jobdata_t* jobdata = ud;
    sk_service_t*  svc      = jobdata->service;
    sk_service_job job      = jobdata->job;
    void*          udata    = jobdata->ud;
    uint32_t       interval = jobdata->interval;

    sk_srv_task_t task;
    memset(&task, 0, sizeof(task));

    task.base.type          = SK_QUEUE_ELEM_WRITE;
    task.type               = SK_SRV_TASK_TIMER;
    task.io_status          = SK_SRV_IO_STATUS_OK;
    task.data.timer.service = svc;
    task.data.timer.job     = job;
    task.data.timer.ud      = udata;
    task.data.timer.valid   = valid;

    // push to service task queue
    sk_srv_status_t ret = sk_service_push_task(svc, &task);
    SK_ASSERT(ret == SK_SRV_STATUS_OK);

    // 2. Clean up jobdata
    free(jobdata);

    // 3. If the interval is not zero, create a new timer for it
    if (!interval) {
        sk_print("interval is 0, skip to create next timer\n");
        return;
    }

    if (!valid) {
        sk_print("timer is invalid, skip to create next one");
        return;
    }

    int ret_code = _timerjob_create(svc, entity, _timer_triggered, interval,
                                    interval, job, udata);
    SK_ASSERT(ret_code == 0);
}

// This proto is ran in master thread
static
int _run (sk_sched_t* sched, sk_entity_t* entity, sk_txn_t* txn,
          void* proto_msg)
{
    TimerEmit* timer_emit_msg = proto_msg;
    sk_service_t*  svc      = (sk_service_t*) (uintptr_t) timer_emit_msg->svc;
    uint32_t       delayed  = timer_emit_msg->delayed;
    uint32_t       interval = timer_emit_msg->interval;
    void*          udata    = (void*) (uintptr_t) timer_emit_msg->udata;
    sk_service_job ujob     = * (sk_service_job*) timer_emit_msg->job.data;

    int ret = _timerjob_create(svc, entity, _timer_triggered,
                               delayed, interval, ujob, udata);
    SK_ASSERT(!ret);
    return 0;
}

sk_proto_t sk_pto_timer_emit = {
    .priority = SK_PTO_PRI_9,
    .descriptor = &timer_emit__descriptor,
    .run = _run
};
