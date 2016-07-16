#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "flibs/fhash.h"
#include "flibs/fmbuf.h"
#include "api/sk_utils.h"
#include "api/sk_env.h"
#include "api/sk_pto.h"
#include "api/sk_metrics.h"
#include "api/sk_timer_service.h"
#include "api/sk_entity_mgr.h"
#include "api/sk_entity_util.h"
#include "api/sk_service.h"

#define SK_SRV_TASK_SZ    (sizeof(sk_srv_task_t))
#define SK_SRV_MAX_TASK   1024

struct sk_service_t {
    int running_task_cnt;
    int _reserved;

    const char*       name; // a ref
    sk_service_opt_t  opt;
    const sk_service_cfg_t* cfg;

    sk_queue_t* pending_tasks; // This queue is only avaliable in master
    sk_srv_data_t* data;
    fhash* apis; // key: api name; value: sk_service_api_t
};

// Internal APIs of service
static
sk_srv_status_t _sk_service_pop_task(sk_service_t* service, sk_srv_task_t* task)
{
    SK_ASSERT(service);
    SK_ASSERT(task);

    sk_queue_t* pending_queue = service->pending_tasks;
    if (sk_queue_empty(pending_queue)) {
        return SK_SRV_STATUS_IDLE;
    }

    // The service is ready to pop, just pop task(s)
    size_t pulled_cnt = sk_queue_pull(pending_queue, SK_QUEUE_ELEM(task), 1);

    if (pulled_cnt > 0) {
        return SK_SRV_STATUS_OK;
    } else {
        return SK_SRV_STATUS_PENDING;
    }
}

static
void _sk_service_handle_exception(sk_service_t* service, sk_srv_status_t st)
{
    switch (st) {
    case SK_SRV_STATUS_OK:
        SK_LOG_TRACE(SK_ENV_LOGGER, "service call ok");
        break;
    case SK_SRV_STATUS_PENDING:
        SK_LOG_TRACE(SK_ENV_LOGGER, "service is pending");
        break;
    case SK_SRV_STATUS_IDLE:
        SK_LOG_TRACE(SK_ENV_LOGGER, "service is idle");
        break;
    default:
        SK_LOG_FATAL(SK_ENV_LOGGER, "Invalid exception status occured");
        SK_ASSERT_MSG(0, "Invalid exception status occured\n");
        break;
    }
}

static
void _sk_service_data_create(sk_service_t* service, sk_srv_data_mode_t mode)
{
    service->data = sk_srv_data_create(mode);

    sk_queue_mode_t queue_mode = 0;
    switch (mode) {
    case SK_SRV_DATA_MODE_RW_PR:
        queue_mode = SK_QUEUE_RW_PR;
        break;
    case SK_SRV_DATA_MODE_RW_PW:
        queue_mode = SK_QUEUE_RW_PW;
        break;
    default:
        SK_ASSERT(0);
    }

    service->pending_tasks = sk_queue_create(queue_mode, SK_SRV_TASK_SZ,
                                             0, SK_SRV_MAX_TASK);
}

static
void _destroy_apis(fhash* apis)
{
    fhash_str_iter api_iter = fhash_str_iter_new(apis);
    sk_service_api_t* api = NULL;

    while ((api = fhash_str_next(&api_iter))) {
        free(api);
    }
    fhash_str_iter_release(&api_iter);
    fhash_str_delete(apis);
}

static
sk_sched_t* _find_target_sched(sk_sched_t* src, int bidx)
{
    sk_sched_t* target = NULL;

    if (bidx == 0) {
        target = src;
    } else if (bidx > 0) {
        SK_ASSERT(bidx <= SK_ENV_CORE->config->bio_cnt);

        sk_engine_t* bio = sk_core_bio(SK_ENV_CORE, bidx);
        target = bio->sched;
    } else {
        // use the bio-1
        target = sk_core_bio(SK_ENV_CORE, 1)->sched;
    }

    return target;
}

static
void _schedule_api_task(sk_service_t* service, const sk_srv_task_t* task)
{
    // 1. Construct protocol
    sk_sched_t* src     = task->src;
    int         bidx    = task->bidx;
    uint64_t    task_id = task->data.api.task_id;
    sk_txn_t*   txn     = task->data.api.txn;
    sk_txn_taskdata_t* taskdata = task->data.api.txn_task;

    ServiceTaskRun task_run_pto = SERVICE_TASK_RUN__INIT;
    task_run_pto.service_name = (char*) sk_service_name(service);
    task_run_pto.api_name     = (char*) task->data.api.name;
    task_run_pto.io_status    = (uint32_t) task->io_status;
    task_run_pto.src          = (uint64_t) (uintptr_t) src;
    task_run_pto.taskdata     = (uint64_t) (uintptr_t) taskdata;

    // 2. Find a bio if needed
    sk_sched_t* target = _find_target_sched(src, bidx);
    SK_ASSERT(target);

    // 3. Deliver the protocol
    sk_sched_send(SK_ENV_SCHED, target, sk_txn_entity(txn), txn,
                  SK_PTO_SVC_TASK_RUN, &task_run_pto, 0);

    sk_print("service: deliver task(%d) to worker\n", (int) task_id);
    SK_LOG_TRACE(SK_ENV_LOGGER, "service: deliver task(%d) to worker",
                 (int) task_id);
}

static
void _schedule_timer_task(sk_service_t* service, const sk_srv_task_t* task)
{
    // 1. Create timer message
    int            bidx   = task->bidx;
    sk_sched_t*    src    = task->src;
    sk_service_job job    = task->data.timer.job;
    sk_entity_t*   entity = task->data.timer.entity;
    sk_obj_t*      ud     = task->data.timer.ud;
    int            valid  = task->data.timer.valid;

    ServiceTimerRun msg = SERVICE_TIMER_RUN__INIT;
    msg.svc       = (uintptr_t) (void*) service;
    msg.job.len   = sizeof (job);
    msg.job.data  = (uint8_t*) &job;
    msg.ud        = (uintptr_t) (void*) ud;
    msg.valid     = valid;

    // 2. Find a bio if needed
    sk_sched_t* target = _find_target_sched(src, bidx);
    SK_ASSERT(target);

    // 3. Schedule to target scheduler to handle the task
    sk_print("schedule timer task: valid: %d\n", valid);
    sk_sched_send(SK_ENV_SCHED, target, entity, NULL,
                      SK_PTO_SVC_TIMER_RUN, &msg, 0);
}

// return 0 on invalid, 1 on valid
static
int _validate_bidx(int idx)
{
    if (idx > 0 && !sk_core_bio(SK_ENV_CORE, idx)) {
        return 0;
    } else if (idx < 0 && !sk_core_bio(SK_ENV_CORE, 1)) {
        return 0;
    } else {
        return 1;
    }
}

typedef struct timer_jobdata_t {
    sk_service_t*   service;
    sk_entity_t*    entity;
    sk_service_job  job;
    const sk_obj_t* ud;
    int             bidx;

#if __WORDSIZE == 64
    int _padding;
#endif
} timer_jobdata_t;

static
void _timer_jobdata_destroy(sk_ud_t ud)
{
    sk_print("destroy timer jobdata\n");
    timer_jobdata_t* jobdata = ud.ud;
    SK_ASSERT(sk_entity_owner(jobdata->entity));
    free(jobdata);
}

static
void _job_triggered(sk_entity_t* entity, int valid, sk_obj_t* ud)
{
    // 1. Extract timer parameters
    sk_print("timer triggered\n");
    timer_jobdata_t* jobdata = sk_obj_get(ud).ud;
    sk_service_t*    svc     = jobdata->service;
    sk_service_job   job     = jobdata->job;
    const sk_obj_t*  udata   = jobdata->ud;
    int              bidx    = jobdata->bidx;

    TimerEmit timer_msg = TIMER_EMIT__INIT;
    timer_msg.svc       = (uint64_t) (uintptr_t) svc;
    timer_msg.job.len   = sizeof (job);
    timer_msg.job.data  = (uint8_t*) &job;
    timer_msg.udata     = (uint64_t) (uintptr_t) udata;
    timer_msg.valid     = valid;
    timer_msg.bidx      = bidx;

    // 4. Send event to prepare running a service task
    sk_sched_send(SK_ENV_SCHED, SK_ENV_CORE->master->sched, entity, NULL,
                    SK_PTO_TIMER_EMIT, &timer_msg, 0);
}

//========================= Public APIs of service =============================
sk_service_t* sk_service_create(const char* service_name,
                                const sk_service_cfg_t* cfg)
{
    if (!service_name) {
        return NULL;
    }

    sk_service_t* service = calloc(1, sizeof(*service));
    service->name = service_name;
    service->cfg  = cfg;
    service->apis = fhash_str_create(0, FHASH_MASK_AUTO_REHASH);

    _sk_service_data_create(service, cfg->data_mode);

    return service;
}

void sk_service_destroy(sk_service_t* service)
{
    if (!service) {
        return;
    }

    sk_queue_destroy(service->pending_tasks);
    sk_srv_data_destroy(service->data);
    _destroy_apis(service->apis);

    free(service);
}

void sk_service_setopt(sk_service_t* service, const sk_service_opt_t opt)
{
    service->opt = opt;
}

sk_service_opt_t* sk_service_opt(sk_service_t* service)
{
    return &service->opt;
}

const sk_service_api_t* sk_service_api(const sk_service_t* service,
                                       const char* api_name)
{
    return fhash_str_get(service->apis, api_name);
}

void sk_service_api_register(sk_service_t* service, const char* api_name,
                             sk_service_api_type_t type)
{
    if (fhash_str_get(service->apis, api_name)) {
        sk_print("service(%s) api: %s has already been registered\n",
                 service->name, api_name);
        return;
    }

    size_t api_name_len = strlen(api_name);
    size_t extra_len = 0;
    if (api_name_len >= sizeof(sk_service_api_type_t)) {
        extra_len = api_name_len - sizeof(sk_service_api_type_t) + 1;
    }

    sk_service_api_t* api = calloc(1, sizeof(*api) + extra_len);
    api->type = type;
    strncpy(api->name, api_name, api_name_len);

    fhash_str_set(service->apis, api_name, api);
}

void sk_service_start(sk_service_t* service)
{
    sk_queue_setstate(service->pending_tasks, SK_QUEUE_STATE_INIT);
    service->opt.init(service, service->opt.srv_data);
    sk_queue_setstate(service->pending_tasks, SK_QUEUE_STATE_IDLE);
}

void sk_service_stop(sk_service_t* service)
{
    sk_queue_setstate(service->pending_tasks, SK_QUEUE_STATE_DESTROY);
    service->opt.release(service, service->opt.srv_data);
}

const char* sk_service_name(const sk_service_t* service)
{
    return service->name;
}

const char* sk_service_type(const sk_service_t* service)
{
    return service->cfg->type;
}

const sk_service_cfg_t* sk_service_config(const sk_service_t* service)
{
    return service->cfg;
}

sk_srv_status_t sk_service_push_task(sk_service_t* service,
                                    const sk_srv_task_t* task)
{
    SK_ASSERT(service);
    SK_ASSERT(task);

    // if the status is not OK before pushing, stop it
    if (task->io_status != SK_SRV_IO_STATUS_OK) {
        return SK_SRV_STATUS_NOT_PUSHED;
    }

    sk_queue_t* pending_queue = service->pending_tasks;
    int ret = sk_queue_push(pending_queue, SK_QUEUE_ELEM(task));
    if (ret) {
        return SK_SRV_STATUS_BUSY;
    }

    return SK_SRV_STATUS_OK;
}

// construct a service_'task_run' protocol, and deliver it to worker thread
void sk_service_schedule_task(sk_service_t* service,
                              const sk_srv_task_t* task)
{
    service->running_task_cnt++;

    switch (task->type) {
    case SK_SRV_TASK_API_QUERY:
        _schedule_api_task(service, task);
        break;
    case SK_SRV_TASK_TIMER:
        _schedule_timer_task(service, task);
        break;
    default:
        SK_ASSERT(0);
    }
}

size_t sk_service_schedule_tasks(sk_service_t* service)
{
    SK_ASSERT(service);
    SK_ASSERT(SK_ENV_ENGINE == SK_ENV_CORE->master);
    sk_print("service schedule tasks\n");

    sk_srv_task_t task;
    sk_srv_status_t pop_status = SK_SRV_STATUS_OK;
    size_t scheduled_task = 0;

    while (true) {
        sk_srv_status_t pop_status = _sk_service_pop_task(service, &task);
        sk_print("pop status: %d\n", pop_status);
        if (pop_status != SK_SRV_STATUS_OK) {
            sk_print("pop break\n");
            break;
        }

        // 1. update the state of task queue
        sk_queue_state_t new_state  = 0;
        sk_queue_state_t curr_state = sk_queue_state(service->pending_tasks);

        switch (task.base.type) {
        case SK_QUEUE_ELEM_READ:
            SK_ASSERT(curr_state == SK_QUEUE_STATE_IDLE ||
                      curr_state == SK_QUEUE_STATE_READ);

            new_state = SK_QUEUE_STATE_READ;
            break;
        case SK_QUEUE_ELEM_WRITE:
            SK_ASSERT(curr_state == SK_QUEUE_STATE_IDLE);

            new_state = SK_QUEUE_STATE_WRITE;
            break;
        default:
            SK_ASSERT(0);
        }

        sk_print("service queue: old state: %d, new state: %d\n", curr_state, new_state);
        sk_queue_setstate(service->pending_tasks, new_state);

        _sk_service_handle_exception(service, pop_status);

        // 2. schedule task to worker
        sk_service_schedule_task(service, &task);
        scheduled_task++;
    }

    // handle last exceptions
    _sk_service_handle_exception(service, pop_status);
    sk_print("service schedule %zu tasks\n", scheduled_task);

    return scheduled_task;
}

void sk_service_task_setcomplete(sk_service_t* service)
{
    SK_ASSERT(service);
    service->running_task_cnt--;

    // reset the state of task queue when there is no running task
    if (service->running_task_cnt == 0) {
        sk_queue_setstate(service->pending_tasks, SK_QUEUE_STATE_IDLE);
    }
}

sk_srv_status_t sk_service_run_iocall(sk_service_t* service,
                                      const sk_txn_t* txn,
                                      sk_txn_taskdata_t* taskdata,
                                      const char* api_name,
                                      sk_srv_io_status_t io_st)
{
    SK_ASSERT(service);
    SK_ASSERT(api_name);

    sk_srv_status_t status = SK_SRV_STATUS_OK;
    void* user_srv_data = service->opt.srv_data;

    int ret = service->opt.iocall(service, txn, user_srv_data, taskdata,
                                   api_name, io_st);
    if (ret) {
        SK_LOG_ERROR(SK_ENV_LOGGER, "service: task failed, service_name: %s "
                     "api_name: %s", service->name, api_name);
        status = SK_SRV_STATUS_TASK_FAILED;
    }

    return status;
}

// Run on worker, run api callback
int sk_service_run_iocall_cb(sk_service_t* service,
                             sk_txn_t* txn,
                             uint64_t task_id,
                             const char* api_name)
{
    SK_ASSERT(service);
    SK_ASSERT(txn);
    SK_ASSERT(api_name);

    void* user_srv_data = service->opt.srv_data;

    return service->opt.iocall_complete(
        service, txn, user_srv_data, task_id, api_name);
}

// Run on worker/bio
void sk_service_api_complete(const sk_service_t* service,
                             const sk_txn_t* txn,
                             sk_txn_taskdata_t* taskdata,
                             const char* api_name)
{
    sk_print("api_complete: taskdata.pending: %u\n", taskdata->pendings);
    if (!sk_txn_task_done(taskdata)) {
        sk_print("sk_txn task has not done, won't trigger workflow\n");
        return;
    }

    // If api task done, schedule it back to caller (a worker) to run task_cb
    sk_print("sk_txn task has all set, trigger task_cb\n");

    sk_entity_t* entity = sk_txn_entity(txn);
    sk_sched_t* api_caller = sk_entity_sched(entity);

    ServiceTaskCb task_cb_msg = SERVICE_TASK_CB__INIT;
    task_cb_msg.taskdata      = (uint64_t) (uintptr_t) taskdata;
    task_cb_msg.service_name  = (char*) service->name;
    task_cb_msg.api_name = (char*) sk_service_api(service, api_name)->name;
    task_cb_msg.task_status   = SK_TXN_TASK_DONE;
    task_cb_msg.svc_task_done = 0; // do not complete the svc task again

    sk_sched_send(SK_ENV_SCHED, api_caller, entity, txn,
                  SK_PTO_SVC_TASK_CB, &task_cb_msg, 0);
}

void* sk_service_data(sk_service_t* service)
{
    SK_ASSERT(service);

    sk_queue_state_t state = sk_queue_state(service->pending_tasks);

    switch (state) {
    case SK_QUEUE_STATE_INIT:
    case SK_QUEUE_STATE_DESTROY:
    case SK_QUEUE_STATE_WRITE:
        return sk_srv_data_get(service->data);
    case SK_QUEUE_STATE_IDLE:
    case SK_QUEUE_STATE_READ:
        SK_LOG_FATAL(SK_ENV_LOGGER, "service %s cannot get mutable data when "
                     "idle or reading data, please correct your logic",
                     service->name);
        SK_ASSERT(0);
        return NULL;
    case SK_QUEUE_STATE_LOCK:
    default:
        SK_ASSERT(0);
    }

    SK_ASSERT(0);
    return NULL;
}

const void* sk_service_data_const(const sk_service_t* service)
{
    SK_ASSERT(service);
    return sk_srv_data_get(service->data);
}

void sk_service_data_set(sk_service_t* service, const void* data)
{
    SK_ASSERT(service);
    sk_queue_state_t state = sk_queue_state(service->pending_tasks);

    switch (state) {
    case SK_QUEUE_STATE_INIT:
    case SK_QUEUE_STATE_DESTROY:
    case SK_QUEUE_STATE_WRITE:
        sk_srv_data_set(service->data, data);
        break;
    case SK_QUEUE_STATE_IDLE:
    case SK_QUEUE_STATE_READ:
        SK_LOG_FATAL(SK_ENV_LOGGER, "service %s cannot set data when "
                     "idle or reading data, please correct your logic",
                     service->name);
        SK_ASSERT(0);
    case SK_QUEUE_STATE_LOCK:
    default:
        SK_ASSERT(0);
    }
}

// User call this api to invoke a service call, it will wrap a iocall
// protocol, and send to master
int sk_service_iocall(sk_service_t* service, sk_txn_t* txn,
                      const char* api_name, const void* req, size_t req_sz,
                      sk_txn_task_cb cb, void* ud, int bidx)
{
    SK_ASSERT(service);
    SK_ASSERT(txn);
    SK_ASSERT(api_name);

    if (!_validate_bidx(bidx)) {
        return 1;
    }

    // 1. checking, the service call must initialize from a module
    SK_ASSERT_MSG(SK_ENV_POS == SK_ENV_POS_MODULE, "Fatal: A service call must \
                  invoke from a module, service_name: %s, api_name: %s\n",
                  service->name, api_name);

    // 2. construct a sk_txn_task and add it
    sk_txn_taskdata_t task_data;
    memset(&task_data, 0, sizeof(task_data));
    task_data.api_name   = sk_service_api(service, api_name)->name;
    task_data.request    = req;
    task_data.request_sz = req_sz;
    task_data.cb         = cb;
    task_data.user_data  = ud;
    task_data.caller_module = sk_txn_current_module(txn);
    task_data.pendings   = 0; // how many internal pending ep calls
    uint64_t task_id = sk_txn_task_add(txn, &task_data);

    // 3. construct iocall protocol
    ServiceIocall iocall_msg = SERVICE_IOCALL__INIT;
    iocall_msg.task_id       = task_id;
    iocall_msg.service_name  = (char*) service->name;
    iocall_msg.api_name      = (char*) api_name;
    iocall_msg.bio_idx       = bidx;
    iocall_msg.txn_task      = (uint64_t) (uintptr_t) sk_txn_taskdata(txn, task_id);

    //5. Send to master engine
    sk_sched_send(SK_ENV_SCHED, SK_ENV_CORE->master->sched,
                  sk_txn_entity(txn), txn,
                  SK_PTO_SVC_IOCALL, &iocall_msg, 0);
    return 0;
}

int sk_service_job_create(sk_service_t*   service,
                          uint32_t        delayed,
                          sk_service_job  job,
                          const sk_obj_t* ud,
                          int             bidx)
{
    SK_ASSERT(service);
    SK_ASSERT(job);

    if (!_validate_bidx(bidx)) {
        return 1;
    }

    sk_print("create a service timer job, bidx: %d\n", bidx);

    // 1. Create timer callback data
    timer_jobdata_t* jobdata = calloc(1, sizeof(*jobdata));
    jobdata->service = service;
    jobdata->entity  = sk_entity_create(NULL);
    jobdata->job     = job;
    jobdata->ud      = ud;
    jobdata->bidx    = bidx;

    sk_entity_mgr_add(SK_ENV_ENTITY_MGR, jobdata->entity);
    sk_entity_timeradd(jobdata->entity, ud);

    sk_ud_t cb_data  = {.ud = jobdata};
    sk_obj_opt_t opt = {.preset = NULL, .destroy = _timer_jobdata_destroy};

    sk_obj_t* param_obj = sk_obj_create(opt, cb_data);

    // 2. Create sk_timer with callback data or create job immediately
    if (delayed > 0) {
        sk_timersvc_t* timersvc = SK_ENV_TMSVC;
        sk_timer_t* timer =
            sk_timersvc_timer_create(
                timersvc, jobdata->entity, delayed, _job_triggered, param_obj);
        SK_ASSERT(timer);
    } else {
        _job_triggered(jobdata->entity, 1, param_obj);
        sk_obj_destroy(param_obj);
    }

    // 3. Record metrics
    sk_metrics_global.srv_timer_emit.inc(1);
    sk_metrics_worker.srv_timer_emit.inc(1);
    return 0;
}
