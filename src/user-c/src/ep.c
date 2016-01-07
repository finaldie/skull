#include <stdlib.h>
#include <string.h>

#include "api/sk_txn.h"
#include "api/sk_env.h"
#include "api/sk_ep_pool.h"
#include "api/sk_service.h"
#include "srv_types.h"
#include "skull/ep.h"

typedef struct ep_job_t {
    skull_ep_handler_t handler;
    skull_service_t    service;
    skull_ep_cb_t      cb;
    void*              ud;
} ep_job_t;

static
size_t _unpack(void* ud, const void* data, size_t len)
{
    ep_job_t* job = ud;
    return job->handler.unpack(job->ud, data, len);
}

static
void _release(void* ud)
{
    if (!ud) return;

    ep_job_t* job = ud;
    job->handler.release(job->ud);
    free(job);
}

static
void _ep_cb(sk_ep_ret_t ret, const void* response, size_t len, void* ud)
{
    // 1. Call user callback
    ep_job_t* job = ud;
    skull_ep_ret_t skull_ret;

    skull_service_t* service = &job->service;

    // TODO: Tricky here, should manually convert the fields one by one
    memcpy(&skull_ret, &ret, sizeof(ret));

    job->cb(skull_ret, response, len, job->ud, service->task->request,
            service->task->response_pb_msg);

    // 2. Reduce pending tasks counts
    job->service.task->pendings--;

    // 3. Try to call api callback
    sk_service_api_complete(service->service, service->txn, service->task,
                            service->task->api_name);
}

static
ep_job_t* _ep_job_create(skull_service_t* service,
                         const skull_ep_handler_t* handler,
                         skull_ep_cb_t cb, const void* ud,
                         sk_ep_handler_t* sk_handler)
{
    // Create ep_job
    ep_job_t* job = calloc(1, sizeof(*job));
    job->handler = *handler;
    job->service = *service;
    job->cb      = cb;
    job->ud      = (void*) ud;

    // Construct sk_ep_handler
    memset(sk_handler, 0, sizeof(*sk_handler));

    // TODO: Tricky here, should manually convert the fields one by one
    memcpy(sk_handler, handler, sizeof(*handler));
    sk_handler->unpack  = _unpack;
    sk_handler->release = _release;
    return job;
}

int skull_ep_send(skull_service_t* service, const skull_ep_handler_t handler,
                  const void* data, size_t count,
                  skull_ep_cb_t cb, void* ud)
{
    // Construct ep job
    sk_ep_handler_t sk_handler;
    ep_job_t* job = _ep_job_create(service, &handler, cb, ud, &sk_handler);

    // Send job to ep pool
    int ret = sk_ep_send(SK_ENV_EP, sk_handler, data, count, _ep_cb, job);
    if (ret == 0) {
        service->task->pendings++;
    } else {
        _release(job);
    }

    return ret;
}
