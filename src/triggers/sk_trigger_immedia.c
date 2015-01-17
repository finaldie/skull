#include <stdlib.h>

#include "api/sk_utils.h"
#include "api/sk_trigger.h"

static
void _trigger_immedia_create(sk_trigger_t* trigger, sk_workflow_cfg_t* cfg)
{
    return;
}

static
void _trigger_immedia_run(sk_trigger_t* trigger)
{
    return;
}

static
void _trigger_immedia_destroy(sk_trigger_t* trigger)
{
    return;
}

sk_trigger_opt_t sk_trigger_immedia = {
    .create = _trigger_immedia_create,
    .run = _trigger_immedia_run,
    .destroy = _trigger_immedia_destroy
};
