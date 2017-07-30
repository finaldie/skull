#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#include "flibs/fev_buff.h"
#include "flibs/fnet.h"

#include "api/sk_utils.h"
#include "api/sk_env.h"
#include "api/sk_pto.h"
#include "api/sk_log.h"
#include "api/sk_entity.h"
#include "api/sk_sched.h"
#include "api/sk_entity_util.h"
#include "api/sk_trigger_utils.h"

static
void _read_cb(fev_state* fev, fev_buff* evbuff, void* arg)
{
    sk_print("stdin read_cb\n");
    sk_entity_t* entity = arg;

    // Check whether error occurred
    if (sk_entity_status(entity) != SK_ENTITY_ACTIVE) {
        sk_print("net entity already has error occurred, won't accept any data\n");
        return;
    }

    ssize_t consumed = sk_trigger_util_unpack(entity);
    if (consumed > 0) {
        sk_trigger_util_deliver(entity, SK_ENV_SCHED, 1);
    }
}

static
void _error(fev_state* fev, fev_buff* evbuff, void* arg)
{
    sk_print("stdin evbuff destroy...\n");
    sk_entity_t* entity = arg;

    sk_entity_safe_destroy(entity);
}

static
int _run(const sk_sched_t* sched, const sk_sched_t* src,
         sk_entity_t* entity, sk_txn_t* txn, void* proto_msg)
{
    sk_print("stdin start event req\n");
    SK_ASSERT(!txn);
    SK_ASSERT(entity);

    int ret = fnet_set_nonblocking(STDIN_FILENO);
    if (ret < 0) {
        sk_print("setup stdin to nonblocking failed, errno: %d\n", errno);
        SK_LOG_FATAL(SK_ENV_LOGGER,
            "setup stdin to nonblocking failed, errno: %d", errno);
        SK_ASSERT(0);

        return 1;
    }

    fev_state* fev = SK_ENV_EVENTLOOP;
    fev_buff* evbuff = fevbuff_new(fev, STDIN_FILENO, _read_cb, _error, entity);
    SK_ASSERT(evbuff);

    sk_entity_stdin_create(entity, evbuff);

    return 0;
}

sk_proto_opt_t sk_pto_stdin_start = {
    .descriptor = &stdin_start__descriptor,
    .run        = _run
};
