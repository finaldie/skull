#include <stdlib.h>
#include <string.h>

#include "api/sk_env.h"
#include "api/sk_core.h"
#include "api/sk_pto.h"
#include "api/sk_utils.h"
#include "api/sk_object.h"
#include "api/sk_log_helper.h"

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
    skull_job_rw_t     rw_type;

    union {
        skull_job_t    job;
        skull_job_np_t job_np;
    } cb_;

    skull_job_udfree_t destroyer;
    void* ud;
} job_data_t;

static
void _job_data_destroy(sk_ud_t ud)
{
    job_data_t* data = ud.ud;

    // TODO: HACKY: Problem is that from service.createJob(...), we create some
    //  data from service scope, but release those data from core scope, which
    //  cause the memory measurement inaccurate:
    //   - Service: Memory usage keep increasing
    //   - Core:    Memory usage keep decreasing
    //
    //  Note: This problem only impact user API layer
    //
    //  How to Fix: Let's see the whole data flow:
    //
    //   ---- service ---------|----------- Core ------------------------------
    //                         |
    //   C-API create_job      |
    //     |    (Data 1)       |
    //     |                   |                     (X ns..)
    //     |-> core.create_job | ----> job trigger ........... entity.destory
    //          (Data 2)                   |                         |
    //                               release (Data 2)                |
    //                                                               |
    //                                                        release (Data 1)
    //
    //  From above, we can see, both data 1 and 2 are created in service scope,
    //   but they all be released in core, which looks like a data transfer from
    //   service to core. (Or a memleak in service layer)
    //
    //  To correctly measure the user timer job memory stat, overall we want to
    //   1) Keep C-API data (Data 1) still remain in service scope
    //   2) Keep core data (Data 2) in core scope
    //
    //  Then we would:
    //   1) core.create_job: When calling it from this file, we reset its
    //      position from 'service' to 'core'
    //
    //   2) c.release_job: When callback happens, reset its position from 'core'
    //      to 'service'
    //
    //  Notes: In the future, needs to find a way move the 'position reset' to
    //         engine.
    sk_service_t* sk_svc = data->svc.service;

    SK_LOG_SETCOOKIE("service.%s", sk_service_name(sk_svc));
    SK_ENV_POS_SAVE(SK_ENV_POS_SERVICE, sk_svc);

    if (data->destroyer) {
        data->destroyer(data->ud);
    }

    free(data);

    SK_ENV_POS_RESTORE();
    SK_LOG_SETCOOKIE(SK_CORE_LOG_COOKIE, NULL);
}

static inline
sk_obj_t* _create_job_arg(job_data_t* jobdata) {
    sk_ud_t      cb_data = {.ud = jobdata};
    sk_obj_opt_t opt     = {.preset = NULL, .destroy = _job_data_destroy};
    sk_obj_t*    param_obj = sk_obj_create(opt, cb_data);

    return param_obj;
}

// TODO: For the hacky description please refer to _job_data_destroy()
static
sk_service_job_ret_t
_create_job(sk_service_t*       service,
                  uint32_t            delayed,
                  sk_service_job_rw_t type,
                  sk_service_job      job,
                  job_data_t*         jobdata,
                  int                 bidx) {

    sk_env_pos_t pos = SK_ENV_POS;

    SK_ASSERT_MSG(pos == SK_ENV_POS_SERVICE || pos == SK_ENV_POS_CORE,
                  "Calling _create_job from unexpected location, "
                  "position: %d\n", pos);

    sk_service_job_ret_t ret = SK_SRV_JOB_OK;

    SK_LOG_SETCOOKIE(SK_CORE_LOG_COOKIE, NULL);
    SK_ENV_POS_SAVE(SK_ENV_POS_CORE, NULL);

    sk_obj_t* arg = _create_job_arg(jobdata);
    ret = sk_service_job_create(service, delayed, type, job, arg, bidx);

    if (ret != SK_SRV_JOB_OK) {
        sk_obj_destroy(arg);
    }

    SK_ENV_POS_RESTORE();
    if (pos == SK_ENV_POS_SERVICE) {
        SK_LOG_SETCOOKIE("service.%s", sk_service_name(service));
    }

    return ret;
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

    skull_job_ret_t skull_ret = SKULL_JOB_OK;
    skull_service_t service   = jobdata->svc;
    service.freezed = 0;
    SK_ASSERT(service.service == sk_svc);

    if (ret != SK_SRV_JOB_OK) {
        skull_ret = SKULL_JOB_ERROR_BUSY;
        service.freezed = 1;
    } else {
        service.freezed = jobdata->rw_type == SKULL_JOB_READ;
    }

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
        // Reset task and txn to NULL, to prevent user to create a pending job
        //  from a no pending job's callback
        service.task = NULL;
        service.txn  = NULL;
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
    jobdata->rw_type   = type;
    jobdata->cb_.job   = job;
    jobdata->destroyer = udfree;
    jobdata->ud        = ud;

    // Fix the bio index to 0 due to we can not schedule it to other thread,
    //  or it would leading a contention issue of *task_data* memory
    sk_service_job_rw_t job_rw = type == SKULL_JOB_READ
                                    ? SK_SRV_JOB_READ : SK_SRV_JOB_WRITE;

    sk_service_job_ret_t sret = _create_job(sk_svc, delayed,
                                    job_rw, _job_cb, jobdata, 0);

    if (sret != SK_SRV_JOB_OK) {
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

    sk_service_job_rw_t job_rw =
        type == SKULL_JOB_READ ? SK_SRV_JOB_READ : SK_SRV_JOB_WRITE;

    sk_service_t* sk_svc = service->service;

    job_data_t* jobdata = calloc(1, sizeof(*jobdata));
    jobdata->svc        = *service;
    jobdata->type       = NOPENDING;
    jobdata->rw_type    = type;
    jobdata->cb_.job_np = job;
    jobdata->destroyer  = udfree;
    jobdata->ud         = ud;

    sk_service_job_ret_t sret =
        _create_job(sk_svc, delayed, job_rw, _job_cb, jobdata, bidx);

    if (sret != SK_SRV_JOB_OK) {
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

const char* skull_service_apiname(const skull_service_t* svc)
{
    sk_txn_taskdata_t* taskdata = svc->task;
    if (!taskdata) {
        return NULL;
    }

    return taskdata->api_name;
}

const char* skull_service_name(const skull_service_t* service)
{
    sk_service_t* svc = service->service;
    return sk_service_name(svc);
}
