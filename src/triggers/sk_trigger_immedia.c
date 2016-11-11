#include <stdlib.h>

#include "api/sk_utils.h"
#include "api/sk_pto.h"
#include "api/sk_trigger.h"

static
void _trigger_immedia_create(sk_trigger_t* trigger)
{
    return;
}

static
void _trigger_immedia_run(sk_trigger_t* trigger)
{
    sk_engine_t* engine = trigger->engine;
    sk_sched_t* sched = engine->sched;
    sk_workflow_t* workflow = trigger->workflow;

    if (0 == sk_workflow_module_cnt(workflow)) {
        sk_print("there is no module for the workflow, skip to run\n");
        return;
    }

    sk_entity_t* entity = sk_entity_create(workflow, SK_ENTITY_NONE);
    sk_txn_t* txn = sk_txn_create(workflow, entity);

    sk_sched_send(sched, NULL, entity, txn, SK_PTO_WORKFLOW_RUN, NULL, 0);
}

static
void _trigger_immedia_destroy(sk_trigger_t* trigger)
{
    return;
}

sk_trigger_opt_t sk_trigger_immedia = {
    .create  = _trigger_immedia_create,
    .run     = _trigger_immedia_run,
    .destroy = _trigger_immedia_destroy
};
