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
#include "api/sk_entity_util.h"
#include "api/sk_trigger_utils.h"

// -----------------------------------------------------------------------------
// EventLoop trigger this callback
// read buffer and create a sk_event
static
void _read_cb(fev_state* fev, fev_buff* evbuff, void* arg)
{
    sk_entity_t* entity = arg;

    // Check whether error occurred
    if (sk_entity_status(entity) != SK_ENTITY_ACTIVE) {
        sk_print("tcp entity already has error occurred, won't accept any data\n");
        return;
    }

    ssize_t consumed = sk_trigger_util_unpack(entity);
    if (consumed > 0) {
        sk_trigger_util_deliver(entity, SK_ENV_SCHED, 1);
    }
}

// Send a destroy msg, so that the scheduler will destroy it later
static
void _error(fev_state* fev, fev_buff* evbuff, void* arg)
{
    sk_print("tcp evbuff destroy...\n");
    sk_entity_t* entity = arg;

    sk_entity_safe_destroy(entity);
}

// register the new sock fd into eventloop
static
int _run(const sk_sched_t* sched, const sk_sched_t* src,
         sk_entity_t* entity, sk_txn_t* txn, void* proto_msg)
{
    sk_print("tcp accept event req\n");
    SK_ASSERT(!txn);
    SK_ASSERT(entity);

    NetAccept* accept_msg = proto_msg;
    int client_fd = accept_msg->fd;
    fev_state* fev = SK_ENV_EVENTLOOP;

    fev_buff* evbuff = fevbuff_new(fev, client_fd, _read_cb, _error, entity);
    SK_ASSERT(evbuff);

    sk_entity_tcp_create(entity, evbuff);

    sk_metrics_worker.accept.inc(1);
    return 0;
}

sk_proto_opt_t sk_pto_net_accept = {
    .descriptor = &net_accept__descriptor,
    .run        = _run
};
