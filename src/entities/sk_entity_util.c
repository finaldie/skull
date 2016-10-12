#include <stdlib.h>
#include <stdio.h>

#include "api/sk_const.h"
#include "api/sk_utils.h"
#include "api/sk_env.h"
#include "api/sk_txn.h"
#include "api/sk_pto.h"
#include "api/sk_log_helper.h"
#include "api/sk_workflow.h"
#include "api/sk_metrics.h"
#include "api/sk_entity_util.h"

void sk_entity_util_unpack(fev_state* fev, fev_buff* evbuff,
                           sk_entity_t* entity)
{
    sk_sched_t* sched = SK_ENV_SCHED;
    sk_workflow_t* workflow = sk_entity_workflow(entity);

    // 1. get the first module, and check whether unpack api exist
    sk_module_t* first_module = sk_workflow_first_module(workflow);
    SK_ASSERT_MSG(first_module->unpack, "missing unpack callback\n");

    // 2. calculate how many bytes we need to read
    size_t read_len = 0;
    size_t used_len = fevbuff_get_usedlen(evbuff, FEVBUFF_TYPE_READ);
    if (used_len == 0) {
        read_len = 1024;
    } else {
        read_len = used_len * 2;
    }

    ssize_t bytes = sk_entity_read(entity, NULL, read_len);
    sk_print("entity read bytes: %ld\n", bytes);
    if (bytes <= 0) {
        sk_print("entity buffer cannot read\n");
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
    const char* module_name = first_module->cfg->name;
    SK_LOG_SETCOOKIE("module.%s", module_name);
    const void* data = fevbuff_rawget(evbuff);
    ssize_t consumed =
        first_module->unpack(first_module->md, txn, data, (size_t)bytes);
    SK_LOG_SETCOOKIE(SK_CORE_LOG_COOKIE, NULL);

    if (consumed == 0) {
        // means user need more data, re-try in next round
        sk_print("user need more data, current data size=%zu\n", bytes);
        return;
    } else if (consumed < 0) { // error occurred
        sk_entity_safe_destroy(entity);
        return;
    }

    // Now, the one user packge has been unpacked successfully
    // 4. set the input data into txn and reset and entity txn pointer
    sk_txn_set_input(txn, data, (size_t)bytes);
    sk_txn_setstate(txn, SK_TXN_UNPACKED);
    sk_entity_sethalftxn(entity, NULL);

    // 4.1 Add transcation log
    sk_txn_log_add(txn, "m:%s:unpack start: %llu end: %llu ",
        module_name, sk_txn_starttime(txn), sk_txn_alivetime(txn));

    // 5. prepare and send a workflow processing event
    sk_sched_send(sched, sched, entity, txn, SK_PTO_WORKFLOW_RUN, NULL, 0);

    // 6. consume the evbuff
    fevbuff_pop(evbuff, (size_t)consumed);

    // 7. update metrics
    sk_metrics_worker.request.inc(1);
    sk_metrics_global.request.inc(1);
}

int sk_entity_safe_destroy(sk_entity_t* entity)
{
    if (!entity) return 0;
    sk_print("sk_entity_safe_destroy: entity %p\n", (void*)entity);

    sk_entity_mgr_t* owner = sk_entity_owner(entity);
    if (!owner) {
        sk_entity_mark(entity, SK_ENTITY_INACTIVE);
        sk_entity_destroy(entity);

        sk_entity_mark(entity, SK_ENTITY_DEAD);
        sk_entity_destroy(entity);
        return 0;
    } else {
        sk_sched_t* sched  = SK_ENV_SCHED;
        sk_sched_t* target = sk_entity_mgr_sched(owner);

        if (sched != target) {
            sk_sched_send(sched, target, entity, NULL,
                      SK_PTO_ENTITY_DESTROY, NULL, 0);
            return 1;
        } else {
            sk_entity_mgr_del(owner, entity);
            return 0;
        }
    }
}

sk_sched_t* sk_entity_sched(const sk_entity_t* entity)
{
    if (!entity) return NULL;

    sk_entity_mgr_t* owner = sk_entity_owner(entity);
    if (!owner) return NULL;

    return sk_entity_mgr_sched(owner);
}
