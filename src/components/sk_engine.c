#include <stdlib.h>
#include <errno.h>

#include "api/sk_utils.h"
#include "api/sk_env.h"
#include "api/sk_const.h"
#include "api/sk_metrics.h"
#include "api/sk_eventloop.h"
#include "api/sk_env.h"
#include "api/sk_core.h"
#include "api/sk_mon.h"
#include "api/sk_log.h"
#include "api/sk_engine.h"

#define SK_ENGINE_UPTIME_TIMER_INTERVAL 1000
#define SK_ENGINE_SNAPSHOT_TIMER_INTERVAL (1000 * 60)

static
sk_timer_t* _create_metrics_timer(sk_engine_t* engine,
                                  uint32_t expiration, // unit: millisecond
                                  sk_timer_triggered timer_cb);

// Triggered every 60 seconds
static
void _snapshot_timer_triggered(sk_entity_t* entity, int valid, sk_obj_t* ud)
{
    // 1. create next timer
    _create_metrics_timer(SK_ENV_ENGINE, SK_ENGINE_SNAPSHOT_TIMER_INTERVAL,
                          _snapshot_timer_triggered);

    // 2. make a snapshot
    sk_core_t* core = SK_ENV_CORE;
    sk_mon_snapshot_all(core);

    // 3. destroy the timer entity
    sk_entity_mark(entity, SK_ENTITY_INACTIVE);
}

// Triggered every 1 second
static
void _uptime_timer_triggered(sk_entity_t* entity, int valid, sk_obj_t* ud)
{
    // 1. create next timer
    _create_metrics_timer(SK_ENV_ENGINE, SK_ENGINE_UPTIME_TIMER_INTERVAL,
                          _uptime_timer_triggered);

    // 2. update uptime
    sk_metrics_global.uptime.inc(1);

    // 3. destroy the timer entity
    sk_entity_mark(entity, SK_ENTITY_INACTIVE);
}

static
void _timer_data_destroy(sk_ud_t ud)
{
    sk_print("metrics timer data destroying...\n");
    sk_entity_t* timer_entity = ud.ud;

    // Clean the entity if it's still no owner
    if (NULL == sk_entity_owner(timer_entity)) {
        sk_entity_destroy(timer_entity);
    }
}

static
sk_timer_t* _create_metrics_timer(sk_engine_t* engine,
                                  uint32_t expiration, // unit: millisecond
                                  sk_timer_triggered timer_cb)
{
    sk_entity_t* timer_entity = sk_entity_create(NULL);
    sk_ud_t cb_data  = {.ud = timer_entity};
    sk_obj_opt_t opt = {.preset = NULL, .destroy = _timer_data_destroy};

    sk_obj_t* param_obj = sk_obj_create(opt, cb_data);

    sk_timer_t* metrics_timer = sk_timersvc_timer_create(engine->timer_svc,
                    timer_entity, expiration, timer_cb, param_obj);

    SK_ASSERT(metrics_timer);
    return metrics_timer;
}

sk_engine_t* sk_engine_create(sk_engine_type_t type)
{
    sk_engine_t* engine = calloc(1, sizeof(*engine));
    engine->type       = type;
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

    // 2. Now, after `sk_thread_env_set`, we can use SK_THREAD_ENV_xxx macros.
    //    Set logger cookie
    sk_logger_setcookie(SK_CORE_LOG_COOKIE);

    // 3. Create a internal timer for metrics update & snapshot
    if (SK_ENV_ENGINE->type == SK_ENGINE_MASTER) {
        sk_print("start uptime metrics timer\n");
        _create_metrics_timer(SK_ENV_ENGINE, SK_ENGINE_UPTIME_TIMER_INTERVAL,
                              _uptime_timer_triggered);

        sk_print("start metrics snapshot timer\n");
        _create_metrics_timer(SK_ENV_ENGINE, SK_ENGINE_SNAPSHOT_TIMER_INTERVAL,
                              _snapshot_timer_triggered);
    }

    // 4. start scheduler
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
