#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "flibs/flist.h"
#include "flibs/fmbuf.h"
#include "flibs/fhash.h"

#include "api/sk_env.h"
#include "api/sk_time.h"
#include "api/sk_sched.h"
#include "api/sk_entity.h"
#include "api/sk_utils.h"
#include "api/sk_mbuf.h"
#include "api/sk_const.h"
#include "api/sk_txn.h"

struct sk_txn_task_t {
    uint64_t id;
    // record the time of start and end
    ulong_t  start;
    ulong_t  end;   // If end is non-zero, means the task is done

    sk_txn_taskdata_t task_data;

    sk_txn_task_status_t status;

    int _padding;
};

struct sk_txn_t {
    sk_workflow_t*  workflow;
    sk_entity_t*    entity;

    void*           input;
    size_t          input_sz;
    fmbuf*          output;
    sk_module_t*    current;
    flist_iter      workflow_idx;
    fhash*          task_tbl;   // key: task_id, value: sk_txn_task_t

    // If the txn has error occurred, then this field will be set to 1
    uint32_t        error     : 1;
    uint32_t        __padding : 31;

    int             __padding1;

    uint64_t        latest_taskid;
    int             total_tasks;
    int             complete_tasks;

    // Unit: micro-second
    ulong_t         start_time;

    sk_txn_state_t  state;

    int             pending_tasks;
    void*           udata;

    // Store the detail transactions, which is used for writing the
    // transaction log
    sk_mbuf_t*      transaction;
};

// Internal APIs
static
sk_txn_task_t* _sk_txn_task_create(
    sk_txn_t* txn, sk_txn_taskdata_t* task_data)
{
    uint64_t task_id = txn->latest_taskid++;

    sk_txn_task_t* task = calloc(1, sizeof(*task));
    task->id    = task_id;
    task->start = sk_time_us();

    if (task_data) {
        task->task_data = *task_data;
        task->task_data.task_id = task_id;
    }

    task->status = SK_TXN_TASK_RUNNING;

    // update the taskdata owner
    task->task_data.owner = task;
    return task;
}

static
void _sk_txn_task_destroy(sk_txn_task_t* task)
{
    if (!task) {
        return;
    }

    // Notes: here we won't release the task_data.request/response,
    //  the responsibility is from user
    free(task);
}

// Public APIs
sk_txn_t* sk_txn_create(sk_workflow_t* wf, sk_entity_t* entity)
{
    sk_txn_t* txn = calloc(1, sizeof(*txn));
    txn->workflow      = wf;
    txn->entity        = entity;
    txn->output        = fmbuf_create((size_t)SK_MAX(wf->cfg->txn_out_sz, 0));
    txn->workflow_idx  = flist_new_iter(wf->modules);
    txn->task_tbl      = fhash_u64_create(0, FHASH_MASK_AUTO_REHASH);
    txn->latest_taskid = 0;
    txn->start_time    = sk_time_us();
    txn->state         = SK_TXN_INIT;
    txn->transaction   = sk_mbuf_create(SK_TXN_DEFAULT_INIT_SIZE,
                                        SK_TXN_DEFAULT_MAX_SIZE);

    // Notes: Here we won't add txn into entity directly, since the newly txn
    //         may need to unpack data first, then it will be a valid txn,
    //         otherwise it will be a half txn in entity
    return txn;
}

int sk_txn_safe_destroy(sk_txn_t* txn)
{
    if (!txn) {
        return 0;
    }

    if (txn->complete_tasks < txn->total_tasks) {
        sk_print("sk_txn_safe_destroy: complete_tasks(%d) < total_tasks(%d), "
                 "won't destroy\n", txn->complete_tasks, txn->total_tasks);
        return 1;
    }

    sk_txn_destroy(txn);
    return 0;
}

void sk_txn_destroy(sk_txn_t* txn)
{
    if (!txn) {
        return;
    }

    sk_entity_txndel(txn->entity, txn);
    fmbuf_delete(txn->output);
    free(txn->input);

    // clean up all the tasks
    fhash_u64_iter task_iter = fhash_u64_iter_new(txn->task_tbl);
    sk_txn_task_t* task = NULL;
    while ((task = fhash_u64_next(&task_iter))) {
        _sk_txn_task_destroy(task);
    }

    fhash_u64_delete(txn->task_tbl);

    // Release the transactions
    sk_mbuf_destroy(txn->transaction);

    free(txn);
}

// Debugging Api
void sk_txn_dump_tasks(const sk_txn_t* txn)
{
    fhash_u64_iter task_iter = fhash_u64_iter_new(txn->task_tbl);
    sk_txn_task_t* task = NULL;
    while ((task = fhash_u64_next(&task_iter))) {
        sk_print(" - txn task: id: %d, apiname: %s, request_sz: %zu,"
                 " response_sz: %zu, status: %d, start: %lu, end: %lu,"
                 " pendings: %u\n",
            (int)task->id, task->task_data.api_name, task->task_data.request_sz,
            task->task_data.response_sz, task->status, task->start,
            task->end, task->task_data.pendings);
    }
}

const void* sk_txn_input(const sk_txn_t* txn, size_t* sz)
{
    *sz = txn->input_sz;
    return txn->input;
}

void sk_txn_set_input(sk_txn_t* txn, const void* data, size_t sz)
{
    if (!sz || !data) {
        return;
    }

    txn->input = calloc(1, sz);
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

        // TODO: (HACKY) The problem is txn.output was be created from core, but
        //  be realloced in module, which cause the memory stat measurement
        //  not accurate. So we force it be marked in core even realloc happens.
        //
        // Note: In current design (at least version <= 1.2.1), txn.append
        //  only can be called from a module.
        SK_ENV_POS_SAVE(SK_ENV_POS_CORE, NULL);

        txn->output = fmbuf_realloc(txn->output, new_sz);

        SK_ENV_POS_RESTORE();
    }

    int ret = fmbuf_push(txn->output, data, sz);
    SK_ASSERT(!ret);
}

const void* sk_txn_output(const sk_txn_t* txn, size_t* sz)
{
    *sz = fmbuf_used(txn->output);
    return fmbuf_head(txn->output);
}

struct sk_workflow_t* sk_txn_workflow(const sk_txn_t* txn)
{
    return txn->workflow;
}

struct sk_entity_t* sk_txn_entity(const sk_txn_t* txn)
{
    return txn->entity;
}

struct sk_module_t* sk_txn_next_module(sk_txn_t* txn)
{
    sk_module_t* module = flist_each(&txn->workflow_idx);
    txn->current = module;
    return module;
}

struct sk_module_t* sk_txn_current_module(const sk_txn_t* txn)
{
    return txn->current;
}

int sk_txn_is_first_module(const sk_txn_t* txn)
{
    return txn->current == sk_workflow_first_module(txn->workflow);
}

int sk_txn_is_last_module(const sk_txn_t* txn)
{
    return txn->current == sk_workflow_last_module(txn->workflow);
}

ulong_t sk_txn_starttime(const sk_txn_t* txn) {
    return txn->start_time;
}

ulong_t sk_txn_alivetime(const sk_txn_t* txn)
{
    return sk_time_us() - txn->start_time;
}

void sk_txn_setudata(sk_txn_t* txn, const void* data)
{
    txn->udata = (void*)data;
}

void* sk_txn_udata(const sk_txn_t* txn)
{
    return txn->udata;
}

bool sk_txn_module_complete(const sk_txn_t* txn)
{
    return !txn->pending_tasks;
}

bool sk_txn_alltask_complete(const sk_txn_t* txn)
{
    return txn->complete_tasks == txn->total_tasks;
}

// =============================================================================
uint64_t sk_txn_task_id(const sk_txn_task_t* task)
{
    return task->id;
}

uint64_t sk_txn_task_add(sk_txn_t* txn, sk_txn_taskdata_t* task_data)
{
    sk_txn_task_t* task = _sk_txn_task_create(txn, task_data);
    fhash_u64_set(txn->task_tbl, task->id, task);

    txn->total_tasks++;
    txn->pending_tasks += task_data->cb ? 1 : 0;

    SK_ASSERT(txn->complete_tasks < txn->total_tasks);
    return task->id;
}

int sk_txn_task_done(const sk_txn_taskdata_t* task)
{
    return task->pendings > 0 ? 0 : 1;
}

void sk_txn_task_setcomplete(sk_txn_t* txn, uint64_t task_id,
                          sk_txn_task_status_t status)
{
    SK_ASSERT(txn->complete_tasks < txn->total_tasks);

    sk_txn_task_t* task = fhash_u64_get(txn->task_tbl, task_id);
    SK_ASSERT(task);

    task->end = sk_time_us();
    task->status = status;

    txn->complete_tasks++;
    txn->pending_tasks -= task->task_data.cb ? 1 : 0;
}

sk_txn_task_status_t sk_txn_task_status(const sk_txn_t* txn, uint64_t task_id)
{
    sk_txn_task_t* task = fhash_u64_get(txn->task_tbl, task_id);
    SK_ASSERT(task);

    return task->status;
}

sk_txn_taskdata_t* sk_txn_taskdata(const sk_txn_t* txn, uint64_t task_id)
{
    sk_txn_task_t* task = fhash_u64_get(txn->task_tbl, task_id);
    SK_ASSERT(task);

    return &task->task_data;
}

ulong_t sk_txn_task_starttime(const sk_txn_t* txn, uint64_t task_id) {
    sk_txn_task_t* task = fhash_u64_get(txn->task_tbl, task_id);
    SK_ASSERT(task);

    return task->start;
}

ulong_t sk_txn_task_lifetime(const sk_txn_t* txn, uint64_t task_id)
{
    sk_txn_task_t* task = fhash_u64_get(txn->task_tbl, task_id);
    SK_ASSERT(task);

    if (task->end == 0) {
        return 0;
    }

    return task->end - task->start;
}

ulong_t sk_txn_task_livetime(const sk_txn_t* txn, uint64_t task_id)
{
    sk_txn_task_t* task = fhash_u64_get(txn->task_tbl, task_id);
    SK_ASSERT(task);

    // task has already done
    if (task->status != SK_TXN_TASK_RUNNING) {
        return task->end - task->start;
    } else {
        return sk_time_us() - task->start;
    }
}

void sk_txn_setstate(sk_txn_t* txn, sk_txn_state_t state)
{
    // Do a basic checking first, txn's state cannot be reset when its value =
    //  SK_TXN_DESTROYED
    if (txn->state == SK_TXN_DESTROYED) {
        SK_ASSERT(state == SK_TXN_DESTROYED);
    }

    txn->state = state;

    // Record the error in to flag, since the state will be changed later
    if (state == SK_TXN_ERROR) {
        txn->error = 1;
    }
}

sk_txn_state_t sk_txn_state(const sk_txn_t* txn)
{
    return txn->state;
}

bool sk_txn_error(const sk_txn_t* txn) {
    return txn->error;
}

void sk_txn_log_add(sk_txn_t* txn, const char* fmt, ...)
{
    if (!fmt) return;

    // Format raw txn log
    char content[SK_TXN_DEFAULT_MAX_SIZE];
    memset(content, 0, SK_TXN_DEFAULT_MAX_SIZE);

    va_list ap;
    va_start(ap, fmt);
    int r = vsnprintf(content, SK_TXN_DEFAULT_MAX_SIZE, fmt, ap);
    SK_ASSERT_MSG(r > 0, "Format txn log failed: fmt: %s\n", fmt);
    va_end(ap);

    // Push txn log full content to internal buffer
    int ret = sk_mbuf_push(txn->transaction, content, strlen(content));
    SK_ASSERT_MSG(!ret, "Add txn log failed: content: %s, size: %zu\n",
                  content, strlen(content));
}

void sk_txn_log_end(sk_txn_t* txn) {
    int ret = sk_mbuf_push(txn->transaction, "\0", 1);
    SK_ASSERT_MSG(!ret, "Add txn log endding str failed");
}

const char* sk_txn_log(const sk_txn_t* txn)
{
    return sk_mbuf_rawget(txn->transaction, 0);
}

int sk_txn_timeout(const sk_txn_t* txn)
{
    sk_workflow_t* workflow = sk_txn_workflow(txn);
    int timeout = workflow->cfg->timeout;
    if (timeout <= 0) {
        return 0;
    }

    // Won't be timeout if it's already in error state
    if (sk_txn_state(txn) == SK_TXN_ERROR) {
        return 0;
    }

    ulong_t alivetime_ms = sk_txn_alivetime(txn) / SK_US_PER_MS;
    if (alivetime_ms > (ulong_t)timeout) {
        return 1;
    }

    return 0;
}
