#ifndef SK_ENV_H
#define SK_ENV_H

#include <stdbool.h>

#include "api/sk_const.h"
#include "api/sk_core.h"

// Per-thread data and macros, in most of time, we only need to use these macros
#define SK_ENV              (sk_thread_env())
#define SK_ENV_CORE         (sk_thread_env()->core)

#define SK_ENV_CONFIG       (sk_thread_env()->core->config)
#define SK_ENV_WORKFLOWS    (sk_thread_env()->core->workflows)
#define SK_ENV_LOGGER       (sk_thread_env()->core->logger)
#define SK_ENV_ENGINE       (sk_thread_env()->engine)
#define SK_ENV_SCHED        (sk_thread_env()->engine->sched)
#define SK_ENV_ENTITY_MGR   (sk_thread_env()->engine->entity_mgr)
#define SK_ENV_EVENTLOOP    (sk_thread_env()->engine->evlp)
#define SK_ENV_MON          (sk_thread_env()->engine->mon)
#define SK_ENV_TMSVC        (sk_thread_env()->engine->timer_svc)
#define SK_ENV_MASTER_SCHED (SK_ENV_CORE->master->sched)
#define SK_ENV_EP           (sk_thread_env()->engine->ep_pool)
#define SK_ENV_POS          (sk_thread_env()->pos)
#define SK_ENV_CURRENT      (sk_thread_env()->current)
#define SK_ENV_NAME         (sk_thread_env()->name)

typedef enum sk_env_pos_t {
    SK_ENV_POS_CORE    = 0,
    SK_ENV_POS_MODULE  = 1,
    SK_ENV_POS_SERVICE = 2
} sk_env_pos_t;

typedef struct sk_thread_env_t {
    // ======== public  ========
    sk_core_t*   core;
    sk_engine_t* engine;

    // used for logging or debugging
    char name[SK_ENV_NAME_LEN];

    sk_env_pos_t pos;

    int          _padding;

    // Current module or service
    const void*  current;
} sk_thread_env_t;

void sk_thread_env_init();
void sk_thread_env_set(sk_thread_env_t* env);

sk_thread_env_t* sk_thread_env();
sk_thread_env_t* sk_thread_env_create(sk_core_t* core, sk_engine_t* engine,
                                      const char* fmt, ...);

bool sk_thread_env_ready();

/**
 * These two macros helps to save/restore the pos and current
 *
 * @note Make sure there is no break/return statement between the two macros
 */
#define SK_ENV_POS_SAVE(pos, current) \
    do { \
        sk_env_pos_t ____prev_pos  = SK_ENV_POS; \
        const void*  ____prev_curr = SK_ENV_CURRENT; \
        SK_ENV_POS     = pos; \
        SK_ENV_CURRENT = current

#define SK_ENV_POS_RESTORE() \
        SK_ENV_POS     = ____prev_pos; \
        SK_ENV_CURRENT = ____prev_curr; \
    } while (0)

#endif

