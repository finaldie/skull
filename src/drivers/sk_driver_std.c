#include <stdlib.h>
#include <unistd.h>

#include "api/sk_utils.h"
#include "api/sk_pto.h"
#include "api/sk_driver.h"

static
void _driver_std_create(sk_driver_t* driver) {}

static
void _driver_std_run(sk_driver_t* driver) {
    sk_engine_t*   engine   = driver->engine;
    sk_workflow_t* workflow = driver->workflow;
    sk_entity_t*   entity   = sk_entity_create(workflow, SK_ENTITY_STD);
    sk_sched_t*    sched    = engine->sched;

    sk_sched_send(sched, NULL, entity, NULL, 0, SK_PTO_STD_START);
}

static
void _driver_std_destroy(sk_driver_t* driver) {}

sk_driver_opt_t sk_driver_std = {
    .create  = _driver_std_create,
    .run     = _driver_std_run,
    .destroy = _driver_std_destroy
};
