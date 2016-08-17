#include <stdlib.h>
#include <string.h>

#include "api/sk_core.h"
#include "api/sk_pto.h"
#include "api/sk_utils.h"
#include "api/sk_object.h"

#include "txn_utils.h"
#include "srv_types.h"
#include "skull/txn.h"
#include "skull/service.h"

void  skull_service_data_set (skull_service_t* service, const void* data)
{
    if (service->freezed) {
        return;
    }

    sk_service_t* sk_srv = service->service;
    SK_ASSERT(sk_srv);

    sk_service_data_set(sk_srv, data);
}

void* skull_service_data (skull_service_t* service)
{
    if (service->freezed) {
        return NULL;
    }

    sk_service_t* sk_srv = service->service;
    SK_ASSERT(sk_srv);
    return sk_service_data(sk_srv);
}

const void* skull_service_data_const (const skull_service_t* service)
{
    sk_service_t* sk_srv = service->service;
    SK_ASSERT(sk_srv);
    return sk_service_data_const(sk_srv);
}

typedef enum svc_job_type_t {
    PENDING   = 0,
    NOPENDING = 1
} svc_job_type_t;

typedef struct job_data_t {
    skull_service_t    svc;
    svc_job_type_t     type;

#if __WORDSIZE == 64
    int __padding;
#endif

    union {
        skull_job_t    job;
        skull_job_np_t job_np;
    } cb_;

    skull_job_udfree_t destroyer;
    void* ud;
} job_data_t;

static
void _timer_data_destroy(sk_ud_t ud)
{
    job_data_t* data = ud.ud;

    if (data->destroyer) {
        data->destroyer(data->ud);
    }

    free(data);
}

static
void _job_cb (sk_service_t* sk_svc, sk_service_job_ret_t ret,
              sk_obj_t* ud, int valid)
{
    sk_print("skull service: timer cb, valid: %d\n", valid);
    job_data_t* jobdata = sk_obj_get(ud).ud;
    SK_ASSERT_MSG(valid, "Currently we won't cancel any user timer, it must be something wrong");

    SK_ASSERT_MSG(ret == SK_SRV_JOB_OK || ret == SK_SRV_JOB_ERROR_BUSY,
                          "ret: %d\n", ret);

    if (!valid) {
        sk_print("Error: skull serivce: timer is not valid, ignore it\n");
        return;
    }

    skull_service_t service = jobdata->svc;
    service.freezed = 0;

    skull_job_ret_t skull_ret =
        ret == SK_SRV_JOB_OK ? SKULL_JOB_OK : SKULL_JOB_ERROR_BUSY;

    if (jobdata->type == PENDING) {
        sk_txn_taskdata_t* task_data = service.task;
        SK_ASSERT(task_data);
        SK_ASSERT(service.txn);

        if (jobdata->cb_.job) {
            jobdata->cb_.job(&service, skull_ret, jobdata->ud,
                             task_data->request, task_data->request_sz,
                             task_data->response, task_data->response_sz);
        }

        // Reduce pending tasks counts
        task_data->pendings--;
        sk_print("service task pending cnt: %u\n", task_data->pendings);

        // Try to call api callback
        sk_service_api_complete(sk_svc, service.txn,
                                task_data, task_data->api_name);
    } else {
        jobdata->cb_.job_np(&service, skull_ret, jobdata->ud);
    }
}

skull_job_ret_t
skull_service_job_create(skull_service_t* service, uint32_t delayed,
                             skull_job_rw_t type, skull_job_t job,
                             void* ud, skull_job_udfree_t udfree)
{
    skull_job_ret_t ret = SKULL_JOB_OK;

    if (!service->task || !service->txn) {
        sk_print("Error: skull service: a pending service job only can be created in a service api\n");
        ret = SKULL_JOB_ERROR_ENV;
        goto create_job_error;
    }

    if (!job) {
        sk_print("Error: no job specificed, make sure to pass a job function here");
        ret = SKULL_JOB_ERROR_NOCB;
        goto create_job_error;
    }

    sk_service_t* sk_svc = service->service;

    job_data_t* jobdata = calloc(1, sizeof(*jobdata));
    jobdata->svc       = *service;
    jobdata->type      = PENDING;
    jobdata->cb_.job   = job;
    jobdata->destroyer = udfree;
    jobdata->ud        = ud;

    sk_ud_t      cb_data = {.ud = jobdata};
    sk_obj_opt_t opt     = {.preset = NULL, .destroy = _timer_data_destroy};
    sk_obj_t*    param_obj = sk_obj_create(opt, cb_data);

    // Fix the bio index to 0 due to we can not schedule it to other thread,
    //  or it would leading a contention issue of *task_data* memory
    sk_service_job_rw_t job_rw = type == SKULL_JOB_READ
                                    ? SK_SRV_JOB_READ : SK_SRV_JOB_WRITE;
    sk_service_job_ret_t sret = sk_service_job_create(sk_svc, delayed,
                                    job_rw, _job_cb, param_obj, 0);
    if (sret != SK_SRV_JOB_OK) {
        sk_obj_destroy(param_obj);
        ret = SKULL_JOB_ERROR_BIO;
    } else {
        service->task->pendings++;
        sk_print("service task pending cnt: %u\n", service->task->pendings);
    }

    return ret;

create_job_error:
    if (udfree) udfree(ud);
    return 1;
}

skull_job_ret_t
skull_service_job_create_np(skull_service_t* service, uint32_t delayed,
                                skull_job_rw_t type, skull_job_np_t job,
                                void* ud, skull_job_udfree_t udfree, int bidx)
{
    skull_job_ret_t ret = SKULL_JOB_OK;

    if (!job) {
        sk_print("Error: no job specificed, make sure to pass a job into here");
        if (udfree) udfree(ud);
        return SKULL_JOB_ERROR_NOCB;
    }

    sk_service_t* sk_svc = service->service;

    job_data_t* jobdata = calloc(1, sizeof(*jobdata));
    jobdata->svc        = *service;
    jobdata->type       = NOPENDING;
    jobdata->cb_.job_np = job;
    jobdata->destroyer  = udfree;
    jobdata->ud         = ud;

    sk_ud_t      cb_data = {.ud = jobdata};
    sk_obj_opt_t opt     = {.preset = NULL, .destroy = _timer_data_destroy};
    sk_obj_t*    param_obj = sk_obj_create(opt, cb_data);

    sk_service_job_rw_t job_rw =
        type == SKULL_JOB_READ ? SK_SRV_JOB_READ : SK_SRV_JOB_WRITE;
    sk_service_job_ret_t sret =
        sk_service_job_create(sk_svc, delayed, job_rw, _job_cb, param_obj, bidx);
    if (sret != SK_SRV_JOB_OK) {
        sk_obj_destroy(param_obj);
        ret = SKULL_JOB_ERROR_BIO;
    }

    return ret;
}

int skull_service_apidata_set(skull_service_t* svc, int type,
                              const void* data, size_t sz)
{
    sk_txn_taskdata_t* taskdata = svc->task;
    if (!taskdata) {
        return 1;
    }

    if (type == SKULL_API_REQ) {
        taskdata->request    = data;
        taskdata->request_sz = sz;
    } else {
        taskdata->response    = (void*)data;
        taskdata->response_sz = sz;
    }

    return 0;
}

void* skull_service_apidata(skull_service_t* svc, int type, size_t* sz)
{
    sk_txn_taskdata_t* taskdata = svc->task;
    if (!taskdata) {
        return NULL;
    }

    if (type == SKULL_API_REQ) {
        if (sz) *sz = taskdata->request_sz;
        return (void*)taskdata->request;
    } else {
        if (sz) *sz = taskdata->response_sz;
        return taskdata->response;
    }
}

const char* skull_service_name(const skull_service_t* service)
{
    sk_service_t* svc = service->service;
    return sk_service_name(svc);
}
