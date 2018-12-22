#ifndef SK_SERVICE_H
#define SK_SERVICE_H

#include "api/sk_malloc.h"
#include "api/sk_config.h"
#include "api/sk_txn.h"
#include "api/sk_sched.h"
#include "api/sk_queue.h"
#include "api/sk_object.h"
#include "api/sk_entity.h"
#include "api/sk_service_data.h"

typedef struct sk_service_t sk_service_t;

typedef enum sk_service_job_ret_t {
    SK_SRV_JOB_OK         = 0,
    SK_SRV_JOB_ERROR_BIO  = 1,
    SK_SRV_JOB_ERROR_BUSY = 2
} sk_service_job_ret_t;

typedef enum sk_service_job_rw_t {
    SK_SRV_JOB_READ  = 1,
    SK_SRV_JOB_WRITE = 2
} sk_service_job_rw_t;

typedef void (*sk_service_job) (sk_service_t*, sk_service_job_ret_t,
                                sk_obj_t* ud, int valid);

// service status
typedef enum sk_srv_status_t {
    SK_SRV_STATUS_OK               = 0,

    // for user
    SK_SRV_STATUS_BUSY             = 1, // error: cannot push new task

    // internal
    SK_SRV_STATUS_TASK_FAILED      = 2,
    SK_SRV_STATUS_PENDING          = 3, // cannot pop task (pending tasks)
    SK_SRV_STATUS_IDLE             = 4, // cannot pop task (there is no task)
    SK_SRV_STATUS_NOT_PUSHED       = 5
} sk_srv_status_t;

// service status for user layer
typedef enum sk_srv_io_status_t {
    SK_SRV_IO_STATUS_OK   = 0,
    SK_SRV_IO_STATUS_BUSY = 1,
    SK_SRV_IO_STATUS_MAX       // DO NOT USE IT
} sk_srv_io_status_t;

typedef enum sk_srv_task_type_t {
    SK_SRV_TASK_API_QUERY = 0,
    SK_SRV_TASK_TIMER = 1
} sk_srv_task_type_t;

typedef struct sk_srv_task_t {
    // Header
    sk_queue_elem_base_t base;  // This must be placed at the front
    sk_srv_task_type_t   type;

    sk_srv_io_status_t   io_status;

    // Body: bio index
    //  (-1)  : Random pick up one
    //  (0)   : Do not use bio
    //  (> 0) : find and run the task on the idx of bio
    int bidx;

    // Body: source scheduler of service call
    const sk_sched_t*    src;

    // Body Padding
    char _padding[sizeof(void*)];

    // Body: Api or Timer call data
    union {
        struct api {
            sk_service_t*  service;
            sk_txn_t*      txn;
            const char*    name;
            sk_txn_taskdata_t* txn_task;
            uint64_t       task_id;
        } api;

        struct timer {
            sk_entity_t*   entity;
            sk_service_job job;
            sk_obj_t*      ud;
            int            valid;

            int            _padding;
        } timer;
    } data;
} sk_srv_task_t;

typedef struct sk_service_opt_t {
    void* srv_data;

    int  (*init)    (sk_service_t*, void* srv_data);
    void (*release) (sk_service_t*, void* srv_data);

    int  (*iocall)  (sk_service_t*, const sk_txn_t*, void* srv_data,
                     sk_txn_taskdata_t* task_data,
                     const char* api_name, sk_srv_io_status_t ustatus);

    int  (*iocall_complete) (sk_service_t* srv, sk_txn_t* txn, void* sdata,
                                uint64_t task_id, const char* api_name);
} sk_service_opt_t;

typedef enum sk_service_api_type_t {
    SK_SRV_API_READ  = 0,
    SK_SRV_API_WRITE = 1
} sk_service_api_type_t;

typedef struct sk_service_api_t {
    sk_service_api_type_t type;
    char name[sizeof(sk_service_api_type_t)];
} sk_service_api_t;

sk_service_t* sk_service_create(const char* service_name,
                                const sk_service_cfg_t* cfg);
void sk_service_destroy(sk_service_t*);

void sk_service_setopt(sk_service_t*, const sk_service_opt_t opt);
sk_service_opt_t* sk_service_opt(sk_service_t*);

const sk_service_api_t* sk_service_api(const sk_service_t*,
                                       const char* api_name);

void sk_service_api_register(sk_service_t*, const char* api_name,
                             sk_service_api_type_t);

void sk_service_api_complete(const sk_service_t* service,
                             const sk_txn_t* txn,
                             sk_txn_taskdata_t* taskdata,
                             const char* api_name);

int  sk_service_start(sk_service_t*);
void sk_service_stop(sk_service_t*);

const char* sk_service_name(const sk_service_t*);
const char* sk_service_type(const sk_service_t*);
const sk_service_cfg_t* sk_service_config(const sk_service_t*);

// APIs for master
sk_srv_status_t sk_service_push_task(sk_service_t*, const sk_srv_task_t*);
size_t sk_service_schedule_tasks(sk_service_t*);
void sk_service_schedule_task(sk_service_t*, const sk_srv_task_t*);
void sk_service_task_setcomplete(sk_service_t*);
int sk_service_running_taskcnt(const sk_service_t*);

// APIs for worker
sk_srv_status_t sk_service_run_iocall(sk_service_t*, const sk_txn_t* txn,
                                      sk_txn_taskdata_t* taskdata,
                                      const char* api_name,
                                      sk_srv_io_status_t io_status);

int sk_service_run_iocall_cb(sk_service_t* service,
                             sk_txn_t* txn,
                             uint64_t task_id,
                             const char* api_name);

// APIs for user
//  Data APIs (Experimental)
void* sk_service_data(sk_service_t*);
const void* sk_service_data_const(const sk_service_t*);
void sk_service_data_set(sk_service_t*, const void* data);

//  Invoke Service IO call
int sk_service_iocall(sk_service_t*, sk_txn_t* txn, const char* api_name,
                      const void* req, size_t req_sz,
                      sk_txn_task_cb cb, void* ud, int bio_idx);

/**
 * Create a service job
 *
 * @param delayed   delay N milliseconds to start the job
 * @param type      read or write type of job
 * @param job       job function
 * @param ud        user data
 * @param bio_idx   background io idx.
 *                  - (0): Do not use bio
 *                  - (-1): random pick up a bio
 *
 * @return
 *  - SK_SRV_JOB_OK
 *  - SK_SRV_JOB_ERROR_BIO
 */
sk_service_job_ret_t
sk_service_job_create(sk_service_t*       svc,
                      uint32_t            delayed,
                      uint32_t            interval,
                      sk_service_job_rw_t type,
                      sk_service_job      job,
                      const sk_obj_t*     ud,
                      int                 bio_idx);

#endif

