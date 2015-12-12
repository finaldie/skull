#include <stdlib.h>

#include "api/sk_utils.h"
#include "api/sk_trigger.h"

extern sk_trigger_opt_t sk_trigger_sock;
extern sk_trigger_opt_t sk_trigger_immedia;
extern sk_trigger_opt_t sk_trigger_stdin;

sk_trigger_t* sk_trigger_create(sk_engine_t* engine, sk_workflow_t* workflow,
                                sk_workflow_cfg_t* cfg)
{
    sk_trigger_t* trigger = calloc(1, sizeof(*trigger));

    // 1. set type first
    if (cfg->enable_stdin) {
        trigger->type = SK_TRIGGER_BY_STDIN;
    } else if (cfg->port > 0) {
        trigger->type = SK_TRIGGER_BY_SOCK;
    } else {
        trigger->type = SK_TRIGGER_IMMEDIATELY;
    }

    // 2. init base data
    trigger->engine = engine;
    trigger->workflow = workflow;

    // 3. set opt
    switch (trigger->type) {
    case SK_TRIGGER_IMMEDIATELY:
        trigger->opt = sk_trigger_immedia;
        break;
    case SK_TRIGGER_BY_SOCK:
        trigger->opt = sk_trigger_sock;
        break;
    case SK_TRIGGER_BY_STDIN:
        trigger->opt = sk_trigger_stdin;
        break;
    default:
        SK_ASSERT_MSG(0, "unhandled trigger type: %d\n", trigger->type);
    }

    // 4. run the opt.create to initialize data field
    trigger->opt.create(trigger, cfg);

    return trigger;
}

void sk_trigger_destroy(sk_trigger_t* trigger)
{
    if (!trigger) {
        return;
    }

    trigger->opt.destroy(trigger);
    free(trigger);
}

void sk_trigger_run(sk_trigger_t* trigger)
{
    trigger->opt.run(trigger);
}
