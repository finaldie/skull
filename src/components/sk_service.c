#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "flibs/fmbuf.h"
#include "api/sk_utils.h"
#include "api/sk_env.h"
#include "api/sk_pto.h"
#include "api/sk_service.h"

#define SK_SRV_TASK_SZ    (sizeof(sk_srv_task_t))
#define SK_SRV_MAX_TASK   1024

struct sk_service_t {
    sk_service_type_t type;
    int running_task_cnt;

    const char*       name; // a ref
    sk_service_opt_t  opt;
    const sk_service_cfg_t* cfg;

    sk_queue_t* pending_tasks; // This queue is only avaliable in master
    sk_srv_data_t* data;

    uint64_t last_taskid;
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

    // the service is idle, just pop
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
        SK_LOG_DEBUG(SK_ENV_LOGGER, "service call ok");
        break;
    case SK_SRV_STATUS_PENDING:
        SK_LOG_DEBUG(SK_ENV_LOGGER, "service is pending");
        break;
    case SK_SRV_STATUS_IDLE:
        SK_LOG_DEBUG(SK_ENV_LOGGER, "service is idle");
        break;
    default:
        SK_ASSERT_MSG(0, "Invalid exception status occured\n");
        break;
    }
}

// Public APIs of service
sk_service_t* sk_service_create(const char* service_name,
                                const sk_service_cfg_t* cfg)
{
    if (!service_name) {
        return NULL;
    }

    sk_service_t* service = calloc(1, sizeof(*service));
    service->name = service_name;

    return service;
}

void sk_service_destroy(sk_service_t* service)
{
    if (!service) {
        return;
    }

    sk_queue_destroy(service->pending_tasks);
    free(service);
}

void sk_service_setopt(sk_service_t* service, sk_service_opt_t opt)
{
    service->opt = opt;
}

void sk_service_settype(sk_service_t* service, sk_service_type_t type)
{
    service->type = type;
}

void sk_service_data_construct(sk_service_t* service, sk_srv_data_mode_t mode)
{
    service->data = sk_srv_data_create(mode);

    sk_queue_mode_t queue_mode = 0;
    switch (mode) {
    case SK_SRV_DATA_MODE_EXCLUSIVE:
        queue_mode = SK_QUEUE_EXCLUSIVE;
        break;
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

void sk_service_start(sk_service_t* service)
{
    service->opt.init(service->opt.srv_data);
}

void sk_service_stop(sk_service_t* service)
{
    service->opt.release(service->opt.srv_data);
}

const char* sk_service_name(const sk_service_t* service)
{
    return service->name;
}

sk_service_type_t sk_service_type(const sk_service_t* service)
{
    return service->type;
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

    // construct protocol
    ServiceTaskRun task_run_pto = SERVICE_TASK_RUN__INIT;
    task_run_pto.task_id = service->last_taskid++;
    task_run_pto.service_name = (char*) sk_service_name(service);
    task_run_pto.api_name = (char*) task->api_name;
    task_run_pto.io_status = (uint32_t) task->io_status;
    task_run_pto.request.data = (unsigned char*) task->request;
    task_run_pto.request.len = task->request_sz;

    // deliver the protocol
    sk_sched_send(SK_ENV_SCHED, sk_txn_entity(task->txn), task->txn,
                  SK_PTO_SERVICE_TASK_RUN, &task_run_pto);

    SK_LOG_DEBUG(SK_ENV_LOGGER, "service: deliver task to worker");
}

size_t sk_service_schedule_tasks(sk_service_t* service)
{
    SK_ASSERT(service);

    sk_srv_task_t task;
    sk_srv_status_t pop_status = SK_SRV_STATUS_OK;
    size_t scheduled_task = 0;

    while ((pop_status = _sk_service_pop_task(service, &task)) ==
           SK_SRV_STATUS_OK) {
        // 1. schedule task to worker
        sk_service_schedule_task(service, &task);
        scheduled_task++;

        // 2. update the state of task queue
        sk_queue_state_t new_state = 0;

        switch (task.base.type) {
        case SK_QUEUE_ELEM_EXCLUSIVE:
            new_state = SK_QUEUE_STATE_LOCK;
            break;
        case SK_QUEUE_ELEM_READ:
            new_state = SK_QUEUE_STATE_READ;
            break;
        case SK_QUEUE_ELEM_WRITE:
            new_state = SK_QUEUE_STATE_WRITE;
            break;
        default:
            SK_ASSERT(0);
        }

        sk_queue_setstate(service->pending_tasks, new_state);

        _sk_service_handle_exception(service, pop_status);
    }

    // handle last exceptions
    _sk_service_handle_exception(service, pop_status);

    return scheduled_task;
}

void sk_service_task_complete(sk_service_t* service)
{
    SK_ASSERT(service);
    service->running_task_cnt--;

    // reset the state of task queue when there is no running task
    if (service->running_task_cnt == 0) {
        sk_queue_setstate(service->pending_tasks, SK_QUEUE_STATE_IDLE);
    }
}

sk_srv_status_t sk_service_run_iocall(sk_service_t* service,
                                      const char* api_name,
                                      sk_srv_io_status_t io_st,
                                      const void* req,
                                      size_t req_sz)
{
    SK_ASSERT(service);
    SK_ASSERT(api_name);

    sk_srv_status_t status = SK_SRV_STATUS_OK;
    void* user_srv_data = service->opt.srv_data;

    int ret = service->opt.io_call(user_srv_data, api_name, io_st, req, req_sz);
    if (ret) {
        SK_LOG_ERROR(SK_ENV_LOGGER, "service: task failed, service_name: %s \
                     api_name: %s", service->name, api_name);
        status = SK_SRV_STATUS_TASK_FAILED;
    }

    return status;
}
