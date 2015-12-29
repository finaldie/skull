#include <stdlib.h>
#include <errno.h>

#include "flibs/fev_buff.h"
#include "flibs/fnet.h"

#include "api/sk_utils.h"
#include "api/sk_env.h"
#include "api/sk_pto.h"
#include "api/sk_log.h"
#include "api/sk_entity.h"
#include "api/sk_entity_util.h"
#include "api/sk_sched.h"

static
void _read_cb(fev_state* fev, fev_buff* evbuff, void* arg)
{
    printf("stdin read_cb\n");
    sk_entity_t* entity = arg;
    sk_workflow_t* workflow = sk_entity_workflow(entity);
    int concurrent = workflow->cfg->concurrent;

    // check whether allow concurrent
    if (!concurrent && sk_entity_taskcnt(entity) > 0) {
        sk_print("entity already have running tasks\n");
        return;
    }

    sk_entity_util_unpack(fev, evbuff, entity);
}

static
void _error(fev_state* fev, fev_buff* evbuff, void* arg)
{
    sk_print("stdin evbuff destroy...\n");
    sk_entity_t* entity = arg;

    sk_sched_t* sched = SK_ENV_SCHED;
    sk_sched_send(sched, sched, entity, NULL, SK_PTO_ENTITY_DESTROY, NULL, 0);
}

static
int _run(sk_sched_t* sched, sk_sched_t* src,
         sk_entity_t* entity, sk_txn_t* txn, void* proto_msg)
{
    sk_print("stdin start event req\n");
    SK_ASSERT(!txn);
    SK_ASSERT(entity);

    int ret = fnet_set_nonblocking(STDIN_FILENO);
    if (ret < 0) {
        sk_print("setup stdin to nonblocking failed, errno: %d\n", errno);
        SK_LOG_ERROR(SK_ENV_LOGGER,
            "setup stdin to nonblocking failed, errno: %d", errno);

        return 1;
    }

    fev_state* fev = SK_ENV_EVENTLOOP;
    fev_buff* evbuff = fevbuff_new(fev, STDIN_FILENO, _read_cb, _error, entity);
    SK_ASSERT(evbuff);

    sk_entity_stdin_create(entity, evbuff);

    return 0;
}

sk_proto_t sk_pto_stdin_start = {
    .priority   = SK_PTO_PRI_8,
    .descriptor = &stdin_start__descriptor,
    .run        = _run
};
