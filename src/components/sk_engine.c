#include <stdlib.h>
#include <errno.h>

#include "api/sk_utils.h"
#include "api/sk_eventloop.h"
#include "api/sk_engine.h"

sk_engine_t* sk_engine_create()
{
    sk_engine_t* engine = calloc(1, sizeof(*engine));
    engine->evlp = sk_eventloop_create();
    engine->entity_mgr = sk_entity_mgr_create(65535);
    engine->sched = sk_sched_create(engine->evlp, engine->entity_mgr);
    engine->mon = sk_mon_create();

    return engine;
}

void sk_engine_destroy(sk_engine_t* engine)
{
    if (!engine) {
        return;
    }

    sk_sched_destroy(engine->sched);
    sk_eventloop_destroy(engine->evlp);
    sk_entity_mgr_destroy(engine->entity_mgr);
    sk_mon_destroy(engine->mon);
}

void sk_engine_link(sk_engine_t* src, sk_engine_t* dst)
{
    sk_sched_setup_bridge(src->sched, dst->sched);
}
