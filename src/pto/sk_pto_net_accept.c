#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "fev/fev.h"
#include "fev/fev_buff.h"
#include "api/sk_utils.h"
#include "api/sk_event.h"
#include "api/sk_pto.h"
#include "api/sk_workflow.h"
#include "api/sk_entity.h"
#include "api/sk_txn.h"
#include "api/sk_env.h"
#include "api/sk_sched.h"

// -----------------------------------------------------------------------------

static
void _unpack_data(fev_state* fev, fev_buff* evbuff, sk_txn_t* txn)
{
    sk_sched_t* sched = sk_txn_sched(txn);
    //sk_workflow_t* workflow = sk_txn_workflow(txn);
    sk_entity_t* entity = sk_txn_entity(txn);

    sk_module_t* module = sk_txn_next_module(txn);
    SK_ASSERT_MSG(module->sk_module_unpack, "missing unpack callback\n");

    // calculate how many bytes we need to read
    size_t used_len = fevbuff_get_usedlen(evbuff, FEVBUFF_TYPE_READ);
    int read_len = 0;
    if (used_len == 0) {
        read_len = 1024;
    } else {
        read_len *= used_len * 2;
    }

    int bytes = sk_entity_read(entity, NULL, read_len);
    if (bytes <= 0) {
        sk_print("buffer cannot read\n");
        return;
    }

    sk_txn_set_input(txn, fevbuff_rawget(evbuff), bytes);
    int consumed = module->sk_module_unpack(txn);
    if (consumed == 0) {
        // means user need more data, re-try in next round
        sk_print("user need more data, current data size=%d\n", bytes);
        return;
    }

    NetProc net_proc_msg = NET_PROC__INIT;
    net_proc_msg.data.len = consumed;
    net_proc_msg.data.data = fevbuff_rawget(evbuff);
    sk_sched_push(sched, entity, txn, SK_PTO_NET_PROC, &net_proc_msg);

    // consume the evbuff
    fevbuff_pop(evbuff, bytes);
}

// EventLoop trigger this callback
// read buffer and create a sk_event
// TODO: sk_event should use protobuf to store the data
static
void _read_cb(fev_state* fev, fev_buff* evbuff, void* arg)
{
    sk_entity_t* entity = arg;
    sk_sched_t* sched = SK_THREAD_ENV_SCHED;
    sk_workflow_t* workflow = sk_entity_workflow(entity);
    int concurrent = workflow->concurrent;

    // check whether allow concurrent
    if (!concurrent && sk_entity_task_cnt(entity) > 0) {
        sk_print("entity already have running tasks\n");
        return;
    }

    sk_print("create a new transcation\n");
    sk_txn_t* txn = sk_txn_create(sched, workflow, entity);

    _unpack_data(fev, evbuff, txn);
}

// send a destroy msg, so that the scheduler will destory it later
static
void _error(fev_state* fev, fev_buff* evbuff, void* arg)
{
    sk_print("evbuff destroy...\n");
    sk_entity_t* entity = arg;
    sk_sched_t* sched = SK_THREAD_ENV_SCHED;

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
    fev_state* fev = SK_THREAD_ENV_EVENTLOOP;

    fev_buff* evbuff = fevbuff_new(fev, client_fd, _read_cb, _error, entity);
    SK_ASSERT(evbuff);

    sk_net_entity_create(entity, evbuff);
    return 0;
}

sk_proto_t sk_pto_net_accept = {
    .priority = SK_PTO_PRI_8,
    .descriptor = &net_accept__descriptor,
    .run = _run
};
