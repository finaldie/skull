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
    SK_TXN_PACKED    = 5,
    SK_TXN_ERROR     = 6,
    SK_TXN_DESTROYED = 7
} sk_txn_state_t;

typedef struct sk_txn_t sk_txn_t;

sk_txn_t* sk_txn_create(struct sk_workflow_t* workflow,
                        struct sk_entity_t* entity);
void sk_txn_destroy(sk_txn_t* txn);
void sk_txn_safe_destroy(sk_txn_t* txn);

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

// sk_txn's alive time from it be created
unsigned long long sk_txn_alivetime(const sk_txn_t* txn);

// set user data
void sk_txn_setudata(sk_txn_t* txn, void* data);

// get user data
void* sk_txn_udata(const sk_txn_t* txn);

bool sk_txn_module_complete(const sk_txn_t* txn);
bool sk_txn_alltask_complete(const sk_txn_t* txn);

void sk_txn_setstate(sk_txn_t* txn, sk_txn_state_t state);
sk_txn_state_t sk_txn_state(const sk_txn_t* txn);

/**
 * Async io task APIs
 */

typedef struct sk_txn_task_t sk_txn_task_t;

typedef enum sk_txn_task_status_t {
    SK_TXN_TASK_RUNNING = 0,
    SK_TXN_TASK_DONE    = 1,
    SK_TXN_TASK_PENDING = 2,
    SK_TXN_TASK_ERROR   = 3
} sk_txn_task_status_t;

typedef int (*sk_txn_task_cb) ();
typedef struct sk_txn_taskdata_t {
    const sk_txn_task_t* owner;
    const void* request;         // serialized pb-c message data
    size_t      request_sz;
    void *      response_pb_msg; // deserialized pb-c message
    void*       user_data;
    struct sk_module_t* caller_module;
    sk_txn_task_cb cb;

    uint32_t    pendings;       // how many internal pending ep calls

#if __WORDSIZE == 64
    int         _padding;
#endif
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

sk_txn_task_status_t sk_txn_task_status(sk_txn_t*, uint64_t task_id);
sk_txn_taskdata_t* sk_txn_taskdata(sk_txn_t* txn, uint64_t task_id);

uint64_t sk_txn_task_id(const sk_txn_task_t* taskdata);
int sk_txn_task_done(const sk_txn_taskdata_t* taskdata);

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

