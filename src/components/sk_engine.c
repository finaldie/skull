#include <stdlib.h>
#include <errno.h>

#include "api/sk_utils.h"
#include "api/sk_const.h"
#include "api/sk_eventloop.h"
#include "api/sk_env.h"
#include "api/sk_log.h"
#include "api/sk_engine.h"

sk_engine_t* sk_engine_create()
{
    sk_engine_t* engine = calloc(1, sizeof(*engine));
    engine->evlp       = sk_eventloop_create();
    engine->entity_mgr = sk_entity_mgr_create(65535);
    engine->sched      = sk_sched_create(engine->evlp, engine->entity_mgr);
    engine->mon        = sk_mon_create();
    engine->timer_svc  = sk_timersvc_create(engine->evlp);

    return engine;
}

void sk_engine_destroy(sk_engine_t* engine)
{
    if (!engine) {
        return;
    }

    sk_sched_destroy(engine->sched);
    sk_entity_mgr_destroy(engine->entity_mgr);
    sk_timersvc_destroy(engine->timer_svc);
    sk_eventloop_destroy(engine->evlp);
    sk_mon_destroy(engine->mon);
    free(engine);
}

void* _sk_engine_thread(void* arg)
{
    // 1. Set thread_env if exist
    sk_thread_env_t* thread_env = arg;
    if (thread_env) {
        sk_thread_env_set(thread_env);
    }

    // 2. Set logger cookie
    sk_logger_setcookie(SK_CORE_LOG_COOKIE);

    // 3. Now, after `sk_thread_env_set`, we can use SK_THREAD_ENV_xxx macros
    sk_sched_t* sched = SK_ENV_SCHED;
    sk_sched_start(sched);
    return NULL;
}

int sk_engine_start(sk_engine_t* engine, void* env, int new_thread)
{
    if (new_thread) {
        return pthread_create(&engine->io_thread, NULL, _sk_engine_thread, env);
    } else {
        _sk_engine_thread(env);
        return 0;
    }
}

void sk_engine_stop(sk_engine_t* engine)
{
    sk_print("sk engine stop\n");
    if (!engine) {
        return;
    }

    sk_sched_stop(engine->sched);
}

int sk_engine_wait(sk_engine_t* engine)
{
    return pthread_join(engine->io_thread, NULL);
}

void sk_engine_link(sk_engine_t* src, sk_engine_t* dst)
{
    sk_sched_setup_bridge(src->sched, dst->sched);
}
