#include <stdlib.h>
#include <unistd.h>

#include "api/sk_utils.h"
#include "api/sk_pto.h"
#include "api/sk_trigger.h"

static
void _trigger_stdin_create(sk_trigger_t* trigger, sk_workflow_cfg_t* cfg)
{
}

static
void _trigger_stdin_run(sk_trigger_t* trigger)
{
    sk_engine_t*   engine   = trigger->engine;
    sk_workflow_t* workflow = trigger->workflow;
    sk_entity_t*   entity   = sk_entity_create(workflow);
    sk_sched_t*    sched    = engine->sched;

    sk_sched_send(sched, NULL, entity, NULL, SK_PTO_STDIN_START, NULL, 0);
}

static
void _trigger_stdin_destroy(sk_trigger_t* trigger)
{
}

sk_trigger_opt_t sk_trigger_stdin = {
    .create  = _trigger_stdin_create,
    .run     = _trigger_stdin_run,
    .destroy = _trigger_stdin_destroy
};
