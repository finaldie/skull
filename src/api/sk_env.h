#ifndef SK_ENV_H
#define SK_ENV_H

#include <stdbool.h>

#include "api/sk_const.h"
#include "api/sk_core.h"

// Per-thread data and macros, in most of time, we only need to use these macros
#define SK_ENV              (sk_thread_env())
#define SK_ENV_CORE         (SK_ENV->core)
#define SK_ENV_ENGINE       (SK_ENV->engine)
#define SK_ENV_NAME         (SK_ENV->name)
#define SK_ENV_POS          (SK_ENV->pos)
#define SK_ENV_CURRENT      (SK_ENV->current)

#define SK_ENV_CONFIG       (SK_ENV_CORE->config)
#define SK_ENV_WORKFLOWS    (SK_ENV_CORE->workflows)
#define SK_ENV_LOGGER       (SK_ENV_CORE->logger)
#define SK_ENV_LOGGER_DIAG  (SK_ENV_CORE->logger_diag)
#define SK_ENV_SCHED        (SK_ENV_ENGINE->sched)
#define SK_ENV_ENTITY_MGR   (SK_ENV_ENGINE->entity_mgr)
#define SK_ENV_EVENTLOOP    (SK_ENV_ENGINE->evlp)
#define SK_ENV_MON          (SK_ENV_ENGINE->mon)
#define SK_ENV_TMSVC        (SK_ENV_ENGINE->timer_svc)
#define SK_ENV_EP           (SK_ENV_ENGINE->ep_pool)
#define SK_ENV_MASTER_SCHED (SK_ENV_CORE->master->sched)

typedef enum sk_env_pos_t {
    SK_ENV_POS_CORE    = 0,
    SK_ENV_POS_MODULE  = 1,
    SK_ENV_POS_SERVICE = 2
} sk_env_pos_t;

typedef struct sk_thread_env_t {
    // ======== public  ========
    sk_core_t*   core;
    sk_engine_t* engine;

    // Used for logging or debugging
    char name[SK_ENV_NAME_LEN];

    sk_env_pos_t pos;

    int          _padding;

    // Current pointer of module or service
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

