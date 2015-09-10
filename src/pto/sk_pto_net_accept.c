#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "flibs/fev.h"
#include "flibs/fev_buff.h"
#include "api/sk_utils.h"
#include "api/sk_event.h"
#include "api/sk_pto.h"
#include "api/sk_workflow.h"
#include "api/sk_entity.h"
#include "api/sk_txn.h"
#include "api/sk_env.h"
#include "api/sk_metrics.h"
#include "api/sk_sched.h"

// -----------------------------------------------------------------------------

static
void _unpack_data(fev_state* fev, fev_buff* evbuff, sk_entity_t* entity)
{
    sk_sched_t* sched = SK_ENV_SCHED;
    sk_workflow_t* workflow = sk_entity_workflow(entity);

    // 1. get the first module, and try to unpack the data
    sk_module_t* first_module = sk_workflow_first_module(workflow);
    SK_ASSERT_MSG(first_module->unpack, "missing unpack callback\n");

    // 2. calculate how many bytes we need to read
    size_t used_len = fevbuff_get_usedlen(evbuff, FEVBUFF_TYPE_READ);
    size_t read_len = 0;
    if (used_len == 0) {
        read_len = 1024;
    } else {
        read_len = used_len * 2;
    }

    ssize_t bytes = sk_entity_read(entity, NULL, read_len);
    sk_print("read bytes: %ld\n", bytes);
    if (bytes <= 0) {
        sk_print("buffer cannot read\n");
        return;
    }

    // get or create a sk_txn
    sk_txn_t* txn = sk_entity_halftxn(entity);
    if (!txn) {
        sk_print("create a new transcation\n");
        txn = sk_txn_create(workflow, entity);
        SK_ASSERT(txn);
        sk_entity_sethalftxn(entity, txn);
    }

    // 3. try to unpack the user data
    const void* data = fevbuff_rawget(evbuff);
    size_t consumed = first_module->unpack(first_module->md, txn,
                                           data, (size_t)bytes);
    if (consumed == 0) {
        // means user need more data, re-try in next round
        sk_print("user need more data, current data size=%zu\n", bytes);
        return;
    }

    // Now, the one user packge has been unpacked successfully
    // 4. set the input data into txn and reset and entity txn pointer
    sk_txn_set_input(txn, data, (size_t)bytes);
    sk_entity_sethalftxn(entity, NULL);

    // 5. prepare and send a workflow processing event
    sk_sched_push(sched, entity, txn, SK_PTO_WORKFLOW_RUN, NULL);

    // 6. consume the evbuff
    fevbuff_pop(evbuff, consumed);

    // 7. update metrics
    sk_metrics_worker.request.inc(1);
    sk_metrics_global.request.inc(1);
}

// EventLoop trigger this callback
// read buffer and create a sk_event
// TODO: sk_event should use protobuf to store the data
static
void _read_cb(fev_state* fev, fev_buff* evbuff, void* arg)
{
    sk_entity_t* entity = arg;
    sk_workflow_t* workflow = sk_entity_workflow(entity);
    int concurrent = workflow->cfg->concurrent;

    // check whether allow concurrent
    if (!concurrent && sk_entity_task_cnt(entity) > 0) {
        sk_print("entity already have running tasks\n");
        return;
    }

    _unpack_data(fev, evbuff, entity);
}

// send a destroy msg, so that the scheduler will destory it later
static
void _error(fev_state* fev, fev_buff* evbuff, void* arg)
{
    sk_print("evbuff destroy...\n");
    sk_entity_t* entity = arg;
    sk_sched_t* sched = SK_ENV_SCHED;

    NetDestroy destroy_msg = NET_DESTROY__INIT;
    sk_sched_push(sched, entity, NULL, SK_PTO_NET_DESTROY, &destroy_msg);
}

// register the new sock fd into eventloop
static
int _run(sk_sched_t* sched, sk_entity_t* entity, sk_txn_t* txn, void* proto_msg)
{
    sk_print("event req\n");
    SK_ASSERT(!txn);
    SK_ASSERT(entity);

    NetAccept* accept_msg = proto_msg;
    int client_fd = accept_msg->fd;
    fev_state* fev = SK_ENV_EVENTLOOP;

    fev_buff* evbuff = fevbuff_new(fev, client_fd, _read_cb, _error, entity);
    SK_ASSERT(evbuff);

    sk_net_entity_create(entity, evbuff);

    sk_metrics_worker.accept.inc(1);
    sk_metrics_global.connection.inc(1);
    return 0;
}

sk_proto_t sk_pto_net_accept = {
    .priority = SK_PTO_PRI_8,
    .descriptor = &net_accept__descriptor,
    .run = _run
};
