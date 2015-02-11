#include <stdlib.h>
#include <string.h>

#include "flibs/flist.h"
#include "flibs/fmbuf.h"
#include "flibs/ftime.h"

#include "api/sk_workflow.h"
#include "api/sk_sched.h"
#include "api/sk_entity.h"
#include "api/sk_utils.h"
#include "api/sk_txn.h"

struct sk_txn_t {
    sk_sched_t*     sched;
    sk_workflow_t*  workflow;
    sk_entity_t*    entity;

    char*           input;
    size_t          input_sz;
    fmbuf*          output;
    flist_iter      workflow_idx;
    sk_module_t*    current;

    unsigned long long start_time;

    int             is_unpacked;
    int             _reserved;

    void*           udata;
};

sk_txn_t* sk_txn_create(struct sk_sched_t* sched,
                        struct sk_workflow_t* workflow,
                        struct sk_entity_t* entity)
{
    sk_txn_t* txn = calloc(1, sizeof(*txn));
    txn->sched = sched;
    txn->workflow = workflow;
    txn->entity = entity;
    txn->output = fmbuf_create(0);
    txn->workflow_idx = flist_new_iter(workflow->modules);
    txn->start_time = fgettime();

    // update the entity ref
    sk_entity_inc_task_cnt(entity);
    return txn;
}

void sk_txn_destroy(sk_txn_t* txn)
{
    if (!txn) {
        return;
    }

    sk_entity_dec_task_cnt(txn->entity);
    fmbuf_delete(txn->output);
    free(txn->input);
    free(txn);
}

const char* sk_txn_input(sk_txn_t* txn, size_t* sz)
{
    *sz = txn->input_sz;
    return txn->input;
}

void sk_txn_set_input(sk_txn_t* txn, const char* data, size_t sz)
{
    if (!sz || !data) {
        return;
    }

    txn->input = malloc(sz);
    memcpy(txn->input, data, sz);
    txn->input_sz = sz;
}

void sk_txn_output_append(sk_txn_t* txn, const char* data, size_t sz)
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

const char* sk_txn_output(sk_txn_t* txn, size_t* sz)
{
    char tmp;

    *sz = fmbuf_used(txn->output);
    return fmbuf_rawget(txn->output, &tmp, 1);
}

struct sk_sched_t* sk_txn_sched(sk_txn_t* txn)
{
    return txn->sched;
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
    return fgettime() - txn->start_time;
}

void sk_txn_setudata(sk_txn_t* txn, void* data)
{
    txn->udata = data;
}

void* sk_txn_udata(sk_txn_t* txn)
{
    return txn->udata;
}
