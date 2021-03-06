#include <stdlib.h>

#include "api/sk_utils.h"
#include "api/sk_pto.h"
#include "api/sk_driver.h"

static void _driver_app_create(sk_driver_t* driver) {}

static void _driver_app_run(sk_driver_t* driver) {
    sk_engine_t* engine = driver->engine;
    sk_sched_t* sched = engine->sched;
    sk_workflow_t* workflow = driver->workflow;

    if (0 == sk_workflow_module_cnt(workflow)) {
        sk_print("there is no module for the workflow, skip to run\n");
        return;
    }

    sk_entity_t* entity = sk_entity_create(workflow, SK_ENTITY_NONE);
    sk_txn_t* txn = sk_txn_create(workflow, entity);

    // For the 'immediately' driver, adjust the state first
    sk_txn_setstate(txn, SK_TXN_RUNNING);
    sk_entity_txnadd(entity, txn);

    sk_sched_send(sched, NULL, entity, txn, 0, SK_PTO_WORKFLOW_RUN);
}

static void _driver_app_destroy(sk_driver_t* driver) {}

sk_driver_opt_t sk_driver_app = {
    .create  = _driver_app_create,
    .run     = _driver_app_run,
    .destroy = _driver_app_destroy
};
