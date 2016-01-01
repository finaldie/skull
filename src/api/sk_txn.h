#ifndef SK_TXN_H
#define SK_TXN_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

struct sk_workflow_t;
struct sk_entity_t;
struct sk_module_t;

typedef enum sk_txn_state_t {
    SK_TXN_INIT      = 0,
    SK_TXN_UNPACKED  = 1,
    SK_TXN_RUNNING   = 2,
    SK_TXN_PENDING   = 3,
    SK_TXN_COMPLETED = 4,
    SK_TXN_PACKED    = 5
} sk_txn_state_t;

typedef struct sk_txn_t sk_txn_t;

sk_txn_t* sk_txn_create(struct sk_workflow_t* workflow,
                        struct sk_entity_t* entity);
void sk_txn_destroy(sk_txn_t* txn);

const void* sk_txn_input(sk_txn_t* txn, size_t* sz);
void sk_txn_set_input(sk_txn_t* txn, const void* data, size_t sz);

void sk_txn_output_append(sk_txn_t* txn, const void* data, size_t sz);
const void* sk_txn_output(sk_txn_t* txn, size_t* sz);

struct sk_workflow_t* sk_txn_workflow(sk_txn_t* txn);
struct sk_entity_t* sk_txn_entity(sk_txn_t* txn);

struct sk_module_t* sk_txn_next_module(sk_txn_t* txn);
struct sk_module_t* sk_txn_current_module(sk_txn_t* txn);
int sk_txn_is_first_module(sk_txn_t* txn);
int sk_txn_is_last_module(sk_txn_t* txn);

// sk_txn's alive time from it be created
unsigned long long sk_txn_alivetime(sk_txn_t* txn);

// set user data
void sk_txn_setudata(sk_txn_t* txn, void* data);

// get user data
void* sk_txn_udata(sk_txn_t* txn);

bool sk_txn_module_complete(sk_txn_t* txn);

void sk_txn_setstate(sk_txn_t* txn, sk_txn_state_t state);
sk_txn_state_t sk_txn_state(sk_txn_t* txn);

/**
 * Async io task APIs
 */

typedef struct sk_txn_task_t sk_txn_task_t;

typedef enum sk_txn_task_status_t {
    SK_TXN_TASK_RUNNING = 0,
    SK_TXN_TASK_DONE = 1,
    SK_TXN_TASK_ERROR = 2
} sk_txn_task_status_t;

typedef int (*sk_txn_module_cb) ();
typedef struct sk_txn_taskdata_t {
    const void* request;         // serialized pb-c message data
    size_t      request_sz;
    void *      response_pb_msg; // deserialized pb-c message
    void*       user_data;
    sk_txn_module_cb cb;
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

void sk_txn_task_setcomplete(sk_txn_t*, uint64_t task_id, sk_txn_task_status_t);
sk_txn_task_status_t sk_txn_task_status(sk_txn_t*, uint64_t task_id);
sk_txn_taskdata_t* sk_txn_taskdata(sk_txn_t* txn, uint64_t task_id);

/**
 * Get the whole life time of the txn task, unit microsecond
 *
 * @note If the task has not completed, it return 0
 */
unsigned long long sk_txn_task_lifetime(sk_txn_t*, uint64_t task_id);

/**
 * Get the whole live time of the txn task, unit microsecond
 *
 * @note If the task has already completed, the result equals to 'lifetime' api
 */
unsigned long long sk_txn_task_livetime(sk_txn_t*, uint64_t task_id);

#endif

