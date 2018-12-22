#ifndef SKULL_C_SERVICE_H
#define SKULL_C_SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <skull/txn.h>

typedef struct _skull_service_t skull_service_t;

// ===================== APIs and Data Structures for Service ==================
void  skull_service_data_set (skull_service_t*, const void* data);
void* skull_service_data (skull_service_t*);
const void* skull_service_data_const (const skull_service_t*);

// api data type definition
#define SKULL_API_REQ  0
#define SKULL_API_RESP 1

int skull_service_apidata_set(skull_service_t*, int type,
                              const void* data, size_t sz);
void* skull_service_apidata(skull_service_t*, int type, size_t* sz);

// Return api name if have, or return NULL
const char* skull_service_apiname(const skull_service_t*);

const char* skull_service_name(const skull_service_t*);

// =============================== Service Job =================================
typedef enum skull_job_rw_t {
    SKULL_JOB_READ  = 0,
    SKULL_JOB_WRITE = 1
} skull_job_rw_t;

typedef enum skull_job_ret_t {
    SKULL_JOB_OK         = 0,
    SKULL_JOB_ERROR_ENV  = 1,
    SKULL_JOB_ERROR_NOCB = 2,
    SKULL_JOB_ERROR_BIO  = 3,
    SKULL_JOB_ERROR_BUSY = 4
} skull_job_ret_t;

typedef void (*skull_job_t) (skull_service_t*, skull_job_ret_t, void* ud,
                             const void* api_req, size_t api_req_sz,
                             void* api_resp, size_t api_resp_sz);

typedef void (*skull_job_np_t) (skull_service_t*, skull_job_ret_t, void* ud);

typedef void (*skull_job_udfree_t) (void* ud);

/**
 * Create a service job which would pending a service api call
 *
 * @param delayed  unit milliseconds
 * @param type     read or write type of job
 * @param job      job callback function
 * @param ud       user data
 * @param udfree   the function is used for releasing user data after timer be
 *                  finished
 *
 * @return
 *   - SKULL_JOB_OK
 *   - SKULL_JOB_ERROR_ENV
 *   - SKULL_JOB_ERROR_NOCB
 *   - SKULL_JOB_ERROR_BIO
 *   - SKULL_JOB_ERROR_BUSY
 *
 * @note When it be called outside a service api call, it returns failure
 * @note The service job only can be ran on the current thread
 */
skull_job_ret_t
skull_service_job_create(skull_service_t*   svc,
                         uint32_t           delayed,
                         skull_job_rw_t     type,
                         skull_job_t        job,
                         void*              ud,
                         skull_job_udfree_t udfree);

/**
 * Create a service job which does NOT make service api call into pending mode
 *
 * @param delayed  unit milliseconds
 * @param interval unit milliseconds. 0 means not a repeated job
 * @param type     read or write type of job
 * @param job      job callback function
 * @param ud       user data
 * @param udfree   the function is used for releasing user data after timer be
 *                  finished
 * @param bio_idx  background io idx
 *                  - (-1)  : random pick up one
 *                  - (0)   : do not use bio
 *                  - (> 0) : use the idx of bio
 *
 * @return
 *   - SKULL_JOB_OK
 *   - SKULL_JOB_ERROR_ENV
 *   - SKULL_JOB_ERROR_NOCB
 *   - SKULL_JOB_ERROR_BIO
 *   - SKULL_JOB_ERROR_BUSY
 */
skull_job_ret_t
skull_service_job_create_np(skull_service_t*   svc,
                            uint32_t           delayed,
                            uint32_t           interval,
                            skull_job_rw_t     type,
                            skull_job_np_t     job,
                            void*              ud,
                            skull_job_udfree_t udfree,
                            int                bio_idx);

#ifdef __cplusplus
}
#endif

#endif

