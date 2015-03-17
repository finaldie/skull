#ifndef SK_TXN_H
#define SK_TXN_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

struct sk_sched_t;
struct sk_workflow_t;
struct sk_entity_t;
struct sk_module_t;

typedef enum sk_txn_task_status_t {
    SK_TXN_TASK_RUNNING = 0,
    SK_TXN_TASK_DONE = 1,
    SK_TXN_TASK_ERROR = 2
} sk_txn_task_status_t;

typedef struct sk_txn_t sk_txn_t;

sk_txn_t* sk_txn_create(struct sk_sched_t* sched,
                        struct sk_workflow_t* workflow,
                        struct sk_entity_t* entity);
void sk_txn_destroy(sk_txn_t* txn);

const char* sk_txn_input(sk_txn_t* txn, size_t* sz);
void sk_txn_set_input(sk_txn_t* txn, const char* data, size_t sz);

void sk_txn_output_append(sk_txn_t* txn, const char* data, size_t sz);
const char* sk_txn_output(sk_txn_t* txn, size_t* sz);

struct sk_sched_t* sk_txn_sched(sk_txn_t* txn);
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

/**
 * Async io task APIs
 */
void sk_txn_task_add(sk_txn_t*, uint64_t task_id);
void sk_txn_task_setcomplete(sk_txn_t*, uint64_t task_id, sk_txn_task_status_t);
sk_txn_task_status_t sk_txn_task_status(sk_txn_t*, uint64_t task_id);

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

