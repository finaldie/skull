#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "flibs/fmbuf.h"
#include "api/sk_utils.h"
#include "api/sk_env.h"
#include "api/sk_pto.h"
#include "api/sk_service.h"

#define SK_SRV_TASK_SZ (sizeof(sk_srv_task_t))
#define SK_SRV_MAX_TASK 1024

// Internal APIs

// used as the service task queue
typedef struct sk_io_queue_t {
    fmbuf* mq;
    size_t max_sz;
} sk_io_queue_t;

static
sk_io_queue_t* sk_io_queue_create(size_t init_task_cnt, size_t max_task_cnt)
{
    SK_ASSERT(init_task_cnt <= max_task_cnt);

    sk_io_queue_t* queue = calloc(1, sizeof(*queue));
    queue->mq = fmbuf_create(init_task_cnt * SK_SRV_TASK_SZ);
    queue->max_sz = max_task_cnt;
    return queue;
}

static
void sk_io_queue_destroy(sk_io_queue_t* queue)
{
    if (!queue) {
        return;
    }

    fmbuf_delete(queue->mq);
    free(queue);
}

static
size_t _get_new_size(sk_io_queue_t* queue, size_t task_cnt)
{
    fmbuf* mq = queue->mq;
    size_t curr_sz = fmbuf_size(mq);
    size_t used_sz = fmbuf_used(mq);
    size_t new_sz = curr_sz ? curr_sz : 1;

    while (new_sz > 0) {
        new_sz *= 2;

        if (new_sz - used_sz >= task_cnt * SK_SRV_TASK_SZ) {
            break;
        }
    }

    size_t max_buf_sz = queue->max_sz * SK_SRV_TASK_SZ;
    new_sz = new_sz > max_buf_sz ? max_buf_sz : new_sz;
    SK_ASSERT(new_sz > 0);
    return new_sz;
}

#define SK_IO_QUEUE_OK 0
#define SK_IO_QUEUE_FAILED 1

static
int sk_io_queue_push(sk_io_queue_t* queue, const sk_srv_task_t* task)
{
    SK_ASSERT(queue);
    SK_ASSERT(task);

    int ret = fmbuf_push(queue->mq, task, SK_SRV_TASK_SZ);
    if (!ret) {
        return SK_IO_QUEUE_OK;
    }

    size_t current_sz = fmbuf_size(queue->mq);
    size_t new_sz = _get_new_size(queue, 1);
    if (current_sz == new_sz) {
        return SK_IO_QUEUE_FAILED;
    }

    queue->mq = fmbuf_realloc(queue->mq, new_sz);
    ret = fmbuf_push(queue->mq, task, SK_SRV_TASK_SZ);
    SK_ASSERT(!ret);

    return SK_IO_QUEUE_OK;
}

/**
 * Pull the 'task_cnt' tasks from the task queue and with the type 'RO', 'WO' or
 *   'EXCLUSIVE'.
 *
 */
static
int sk_io_queue_pull_bytype(sk_io_queue_t* queue, sk_srv_req_type_t type,
                            int task_cnt, sk_srv_task_t* tasks)
{
    fmbuf* mq = queue->mq;

    sk_srv_task_t tmp_task;
    sk_srv_task_t* task = NULL;
    int pulled_task_cnt = 0;

    while (pulled_task_cnt < task_cnt) {
        task = fmbuf_rawget(mq, &tmp_task, SK_SRV_TASK_SZ);

        if (!task) {
            break;
        }

        // found a un-expected task, stop it
        if (task->req_type != type) {
            break;
        }

        int ret = fmbuf_pop(mq, tasks + pulled_task_cnt, SK_SRV_TASK_SZ);
        SK_ASSERT(!ret);
        pulled_task_cnt++;
    };


    return pulled_task_cnt;
}

static
int sk_io_queue_pull(sk_io_queue_t* queue, int task_cnt, sk_srv_task_t* tasks)
{
    fmbuf* mq = queue->mq;
    int pulled_task_cnt = 0;

    while (pulled_task_cnt < task_cnt) {
        int ret = fmbuf_pop(mq, tasks + pulled_task_cnt, SK_SRV_TASK_SZ);

        if (ret) {
            sk_print("io queue empty, stop pulling it\n");
            break;
        }

        pulled_task_cnt++;
    };


    return pulled_task_cnt;
}

static
bool sk_io_queue_empty(sk_io_queue_t* queue)
{
    return fmbuf_used(queue->mq) == 0;
}

// =============================================================================
// Service APIs

struct sk_service_t {
    sk_service_type_t type;

    // the service running mode which type of task
    sk_srv_req_type_t current_running_mode;

    const char*       name; // a ref
    sk_service_opt_t  opt;
    const sk_service_cfg_t* cfg;

    sk_io_queue_t* pending_tasks; // This queue is only avaliable in master
    sk_srv_data_t* data;

    uint64_t last_taskid;
    int      running_task_cnt;

#if __WORDSIZE == 64
    int      _padding;
#endif
};

// Internal APIs of service
static
int _sk_service_pop_task_bymode(sk_service_t* service, sk_srv_task_t* task)
{
    sk_io_queue_t* pending_queue = service->pending_tasks;
    sk_srv_data_mode_t data_mode = sk_srv_data_mode(service->data);
    int running_task_cnt = service->running_task_cnt;
    int pulled_cnt = 0;

    if (data_mode == SK_SRV_DATA_MODE_EXCLUSIVE) {
        if (running_task_cnt > 0) {
            SK_ASSERT(running_task_cnt == 1);
            return 0;
        }

        pulled_cnt = sk_io_queue_pull(pending_queue, 1, task);
    } else { // RW
        pulled_cnt = sk_io_queue_pull_bytype(pending_queue,
                            service->current_running_mode, 1, task);
    }

    return pulled_cnt;
}

static
sk_srv_status_t _sk_service_pop_task(sk_service_t* service, sk_srv_task_t* task)
{
    SK_ASSERT(service);
    SK_ASSERT(task);

    sk_io_queue_t* pending_queue = service->pending_tasks;
    if (sk_io_queue_empty(pending_queue)) {
        return SK_SRV_STATUS_IDLE;
    }

    int pulled_cnt = 0;

    // the service is idle, just pop
    if (service->running_task_cnt == 0) {
        pulled_cnt = sk_io_queue_pull(pending_queue, 1, task);
        SK_ASSERT(pulled_cnt);
    } else {
        pulled_cnt = _sk_service_pop_task_bymode(service, task);
    }

    if (pulled_cnt > 0) {
        return SK_SRV_STATUS_OK;
    } else {
        return SK_SRV_STATUS_PENDING;
    }

    return 0;
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
    service->pending_tasks = sk_io_queue_create(0, SK_SRV_MAX_TASK);

    return service;
}

void sk_service_destroy(sk_service_t* service)
{
    if (!service) {
        return;
    }

    sk_io_queue_destroy(service->pending_tasks);
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
    if (task->srv_status != SK_SRV_STATUS_OK) {
        return task->srv_status;
    }

    sk_io_queue_t* pending_queue = service->pending_tasks;
    int ret = sk_io_queue_push(pending_queue, task);
    if (ret == SK_IO_QUEUE_FAILED) {
        return SK_SRV_STATUS_BUSY;
    }

    return SK_SRV_STATUS_OK;
}

// construct a service_'task_run' protocol, and deliver it to worker thread
void sk_service_schedule_task(sk_service_t* service,
                              const sk_srv_task_t* task)
{
    service->running_task_cnt++;
    service->current_running_mode = task->req_type;

    // construct protocol
    ServiceTaskRun task_run_pto = SERVICE_TASK_RUN__INIT;
    task_run_pto.task_id = service->last_taskid++;
    task_run_pto.service_name = (char*) sk_service_name(service);
    task_run_pto.api_name = (char*) task->api_name;
    task_run_pto.request.data = (unsigned char*) task->request;
    task_run_pto.request.len = task->request_sz;

    // deliver the protocol
    sk_sched_send(SK_ENV_SCHED, sk_txn_entity(task->txn), task->txn,
                  SK_PTO_SERVICE_TASK_RUN, &task_run_pto);

    SK_LOG_DEBUG(SK_ENV_LOGGER, "service: deliver task to worker");
}

int sk_service_schedule_tasks(sk_service_t* service)
{
    SK_ASSERT(service);

    sk_srv_task_t task;
    sk_srv_status_t pop_status = SK_SRV_STATUS_OK;
    sk_srv_data_mode_t data_mode = sk_srv_data_mode(service->data);

    if (data_mode == SK_SRV_DATA_MODE_EXCLUSIVE) {
        pop_status = _sk_service_pop_task(service, &task);

        if (pop_status == SK_SRV_STATUS_OK) {
            sk_service_schedule_task(service, &task);
        }
    } else {
        while ((pop_status = _sk_service_pop_task(service, &task)) ==
               SK_SRV_STATUS_OK) {
            sk_service_schedule_task(service, &task);
        }
    }

    // handle exceptions
    _sk_service_handle_exception(service, pop_status);

    return 0;
}

void sk_service_task_complete(sk_service_t* service)
{
    SK_ASSERT(service);
    service->running_task_cnt--;
}

sk_srv_status_t sk_service_run_iocall(sk_service_t* service,
                                      const char* api_name,
                                      const void* req,
                                      size_t req_sz)
{
    SK_ASSERT(service);
    SK_ASSERT(api_name);

    sk_srv_status_t status = SK_SRV_STATUS_OK;
    void* user_srv_data = service->opt.srv_data;

    int ret = service->opt.io_call(user_srv_data, api_name, req, req_sz);
    if (ret) {
        SK_LOG_ERROR(SK_ENV_LOGGER, "service: task failed, service_name: %s \
                     api_name: %s", service->name, api_name);
        status = SK_SRV_STATUS_TASK_FAILED;
    }

    return status;
}
