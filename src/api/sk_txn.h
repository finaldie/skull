#ifndef SK_TXN_H
#define SK_TXN_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "api/sk_types.h"

struct sk_workflow_t;
struct sk_entity_t;
struct sk_module_t;

typedef enum sk_txn_state_t {
    SK_TXN_INIT      = 0,
    SK_TXN_UNPACKED  = 1,
    SK_TXN_RUNNING   = 2,
    SK_TXN_PENDING   = 3,
    SK_TXN_COMPLETED = 4,
    SK_TXN_PACKED    = 5,
    SK_TXN_ERROR     = 6,
    SK_TXN_TIMEOUT   = 7,
    SK_TXN_DESTROYED = 8
} sk_txn_state_t;

typedef struct sk_txn_t sk_txn_t;

sk_txn_t* sk_txn_create(struct sk_workflow_t* workflow,
                        struct sk_entity_t* entity);
void sk_txn_destroy(sk_txn_t* txn);
int sk_txn_safe_destroy(sk_txn_t* txn);

const void* sk_txn_input(const sk_txn_t* txn, size_t* sz);
void sk_txn_set_input(sk_txn_t* txn, const void* data, size_t sz);

void sk_txn_output_append(sk_txn_t* txn, const void* data, size_t sz);
const void* sk_txn_output(const sk_txn_t* txn, size_t* sz);

struct sk_workflow_t* sk_txn_workflow(const sk_txn_t* txn);
struct sk_entity_t* sk_txn_entity(const sk_txn_t* txn);

struct sk_module_t* sk_txn_next_module(sk_txn_t* txn);
struct sk_module_t* sk_txn_current_module(const sk_txn_t* txn);
int sk_txn_is_first_module(const sk_txn_t* txn);
int sk_txn_is_last_module(const sk_txn_t* txn);

// sk_txn's alive time from it be created (unit: micro-second)
slong_t sk_txn_alivetime(const sk_txn_t* txn);
slong_t sk_txn_starttime(const sk_txn_t* txn);

int sk_txn_timeout(const sk_txn_t* txn);

// set user data
void sk_txn_setudata(sk_txn_t* txn, const void* data);

// get user data
void* sk_txn_udata(const sk_txn_t* txn);

bool sk_txn_module_complete(const sk_txn_t* txn);
bool sk_txn_alltask_complete(const sk_txn_t* txn);

void sk_txn_setstate(sk_txn_t* txn, sk_txn_state_t state);
sk_txn_state_t sk_txn_state(const sk_txn_t* txn);

// Return whehter txn has error occurred
bool sk_txn_error(const sk_txn_t*);

/**
 * Async io task APIs
 */

typedef struct sk_txn_task_t sk_txn_task_t;

typedef enum sk_txn_task_status_t {
    SK_TXN_TASK_RUNNING = 0,
    SK_TXN_TASK_DONE    = 1,
    SK_TXN_TASK_PENDING = 2,
    SK_TXN_TASK_BUSY    = 3
} sk_txn_task_status_t;

typedef int (*sk_txn_task_cb) ();
typedef struct sk_txn_taskdata_t {
    const sk_txn_task_t* owner;
    const char* api_name;
    const void* request;    // user responsible for release this data
    size_t      request_sz;
    void *      response;   // user responsible for release this data
    size_t      response_sz;
    void*       user_data;
    struct sk_module_t* caller_module;
    sk_txn_task_cb cb;

    uint32_t    pendings;       // how many internal pending ep calls

    int         _padding;

    uint64_t    task_id;
} sk_txn_taskdata_t;

/**
 * Create and add a task into sk_txn
 *
 * @param request  The service iocall request data
 * @param cb       The service iocall callback function
 *
 * @return taskid
 */
uint64_t sk_txn_task_add(sk_txn_t*, sk_txn_taskdata_t* task_data);

// return 0 -> task not complete; 1 -> task complete
void sk_txn_task_setcomplete(sk_txn_t*, uint64_t task_id, sk_txn_task_status_t);

sk_txn_task_status_t sk_txn_task_status(const sk_txn_t*, uint64_t task_id);
sk_txn_taskdata_t* sk_txn_taskdata(const sk_txn_t* txn, uint64_t task_id);

uint64_t sk_txn_task_id(const sk_txn_task_t* taskdata);
int sk_txn_task_done(const sk_txn_taskdata_t* taskdata);

slong_t sk_txn_task_starttime(const sk_txn_t*, uint64_t task_id);

/**
 * Get the whole life time of the txn task, unit microsecond
 *
 * @note If the task has not completed, it return 0
 */
slong_t sk_txn_task_lifetime(const sk_txn_t*, uint64_t task_id);

/**
 * Get the whole live time of the txn task, unit microsecond
 *
 * @note If the task has already completed, the result equals to 'lifetime' api
 */
slong_t sk_txn_task_livetime(const sk_txn_t*, uint64_t task_id);

/**
 * Add a transaction log
 */
void sk_txn_log_add(sk_txn_t*, const char* fmt, ...);
void sk_txn_log_end(sk_txn_t*);

/**
 * Get full transaction log
 */
const char* sk_txn_log(const sk_txn_t*);

// Debugging API
void sk_txn_dump_tasks(const sk_txn_t* txn);

#endif

