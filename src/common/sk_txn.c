#include <stdlib.h>
#include <string.h>

#include "flibs/flist.h"
#include "flibs/fmbuf.h"
#include "flibs/ftime.h"
#include "flibs/fhash.h"

#include "api/sk_sched.h"
#include "api/sk_entity.h"
#include "api/sk_utils.h"
#include "api/sk_txn.h"

struct sk_txn_task_t {
    // record the time of start and end
    unsigned long long start;
    unsigned long long end;   // If end is non-zero, means the task is done

    sk_txn_taskdata_t task_data;

    sk_txn_task_status_t status;

#if __WORDSIZE == 64
    int _padding;
#endif
};

struct sk_txn_t {
    sk_workflow_t*  workflow;
    sk_entity_t*    entity;

    void*           input;
    size_t          input_sz;
    fmbuf*          output;
    flist_iter      workflow_idx;
    sk_module_t*    current;
    fhash*          task_tbl;   // key: task_id, value: sk_txn_task_t

    uint64_t        latest_taskid;
    int             running_tasks;
    int             complete_tasks;

    unsigned long long  start_time;

    sk_txn_state_t  state;
    sk_txn_pos_t    position;

    void*           udata;
};

// Internal APIs
static
sk_txn_task_t* _sk_txn_task_create(sk_txn_taskdata_t* task_data)
{
    sk_txn_task_t* task = calloc(1, sizeof(*task));
    task->start = ftime_gettime();
    if (task_data) {
        task->task_data = *task_data;
    }

    task->status = SK_TXN_TASK_RUNNING;

    return task;
}

static
void _sk_txn_task_destroy(sk_txn_task_t* task)
{
    if (!task) {
        return;
    }

    // Must release the request data if have, or it will be a mem leak
    free((void*)task->task_data.request);
    free(task);
}

// Public APIs
sk_txn_t* sk_txn_create(sk_workflow_t* workflow, sk_entity_t* entity)
{
    sk_txn_t* txn = calloc(1, sizeof(*txn));
    txn->workflow = workflow;
    txn->entity = entity;
    txn->output = fmbuf_create(0);
    txn->workflow_idx = flist_new_iter(workflow->modules);
    txn->task_tbl = fhash_u64_create(0, FHASH_MASK_AUTO_REHASH);
    txn->latest_taskid = 0;
    txn->start_time = ftime_gettime();
    txn->state = SK_TXN_INIT;
    txn->position = SK_TXN_POS_CORE;

    return txn;
}

void sk_txn_destroy(sk_txn_t* txn)
{
    if (!txn) {
        return;
    }

    sk_entity_taskcnt_dec(txn->entity);
    fmbuf_delete(txn->output);
    free(txn->input);

    // clean up all the tasks
    fhash_u64_iter task_iter = fhash_u64_iter_new(txn->task_tbl);
    sk_txn_task_t* task = NULL;
    while ((task = fhash_u64_next(&task_iter))) {
        _sk_txn_task_destroy(task);
    }

    fhash_u64_delete(txn->task_tbl);
    free(txn);
}

const void* sk_txn_input(sk_txn_t* txn, size_t* sz)
{
    *sz = txn->input_sz;
    return txn->input;
}

void sk_txn_set_input(sk_txn_t* txn, const void* data, size_t sz)
{
    if (!sz || !data) {
        return;
    }

    txn->input = malloc(sz);
    memcpy(txn->input, data, sz);
    txn->input_sz = sz;
}

void sk_txn_output_append(sk_txn_t* txn, const void* data, size_t sz)
{
    if (sz == 0) {
        return;
    }

    size_t free_sz = fmbuf_free(txn->output);

    if (free_sz < sz) {
        size_t new_sz = fmbuf_used(txn->output) + sz;
        txn->output = fmbuf_realloc(txn->output, new_sz);
    }

    int ret = fmbuf_push(txn->output, data, sz);
    SK_ASSERT(!ret);
}

const void* sk_txn_output(sk_txn_t* txn, size_t* sz)
{
    *sz = fmbuf_used(txn->output);
    return fmbuf_head(txn->output);
}

struct sk_workflow_t* sk_txn_workflow(sk_txn_t* txn)
{
    return txn->workflow;
}

struct sk_entity_t* sk_txn_entity(sk_txn_t* txn)
{
    return txn->entity;
}

struct sk_module_t* sk_txn_next_module(sk_txn_t* txn)
{
    sk_module_t* module = flist_each(&txn->workflow_idx);
    txn->current = module;
    return module;
}

struct sk_module_t* sk_txn_current_module(sk_txn_t* txn)
{
    return txn->current;
}

int sk_txn_is_first_module(sk_txn_t* txn)
{
    return txn->current == sk_workflow_first_module(txn->workflow);
}

int sk_txn_is_last_module(sk_txn_t* txn)
{
    return txn->current == sk_workflow_last_module(txn->workflow);
}

unsigned long long sk_txn_alivetime(sk_txn_t* txn)
{
    return ftime_gettime() - txn->start_time;
}

void sk_txn_setudata(sk_txn_t* txn, void* data)
{
    txn->udata = data;
}

void* sk_txn_udata(sk_txn_t* txn)
{
    return txn->udata;
}

bool sk_txn_module_complete(sk_txn_t* txn)
{
    if (txn->running_tasks == txn->complete_tasks) {
        return true;
    }

    return false;
}

// =============================================================================
uint64_t sk_txn_task_add(sk_txn_t* txn, sk_txn_taskdata_t* task_data)
{
    sk_txn_task_t* task = _sk_txn_task_create(task_data);
    uint64_t task_id = txn->latest_taskid++;
    fhash_u64_set(txn->task_tbl, task_id, task);
    txn->running_tasks++;

    SK_ASSERT(txn->complete_tasks < txn->running_tasks);
    return task_id;
}

void sk_txn_task_setcomplete(sk_txn_t* txn, uint64_t task_id,
                          sk_txn_task_status_t status)
{
    SK_ASSERT(status != SK_TXN_TASK_RUNNING);
    SK_ASSERT(txn->complete_tasks < txn->running_tasks);

    sk_txn_task_t* task = fhash_u64_get(txn->task_tbl, task_id);
    SK_ASSERT(task);

    task->end = ftime_gettime();
    task->status = status;
    txn->complete_tasks++;
}

sk_txn_task_status_t sk_txn_task_status(sk_txn_t* txn, uint64_t task_id)
{
    sk_txn_task_t* task = fhash_u64_get(txn->task_tbl, task_id);
    SK_ASSERT(task);

    return task->status;
}

sk_txn_taskdata_t* sk_txn_taskdata(sk_txn_t* txn, uint64_t task_id)
{
    sk_txn_task_t* task = fhash_u64_get(txn->task_tbl, task_id);
    SK_ASSERT(task);

    return &task->task_data;
}

unsigned long long sk_txn_task_lifetime(sk_txn_t* txn, uint64_t task_id)
{
    sk_txn_task_t* task = fhash_u64_get(txn->task_tbl, task_id);
    SK_ASSERT(task);

    if (task->end == 0) {
        return 0;
    }

    return task->end - task->start;
}

unsigned long long sk_txn_task_livetime(sk_txn_t* txn, uint64_t task_id)
{
    sk_txn_task_t* task = fhash_u64_get(txn->task_tbl, task_id);
    SK_ASSERT(task);

    // task has already done
    if (task->status != SK_TXN_TASK_RUNNING) {
        return task->end - task->start;
    } else {
        return ftime_gettime() - task->start;
    }
}

void sk_txn_setstate(sk_txn_t* txn, sk_txn_state_t state)
{
    txn->state = state;

    if (state == SK_TXN_UNPACKED) {
        SK_ASSERT(txn->entity);

        // update the entity ref
        sk_entity_taskcnt_inc(txn->entity);
    }
}

sk_txn_state_t sk_txn_state(sk_txn_t* txn)
{
    return txn->state;
}

void sk_txn_setpos(sk_txn_t* txn, sk_txn_pos_t pos)
{
    txn->position = pos;
}

sk_txn_pos_t sk_txn_pos(sk_txn_t* txn)
{
    return txn->position;
}
