#ifndef SK_ENV_H
#define SK_ENV_H

#include "api/sk_core.h"

// per-thread data and macros, most of time, normally you only need to use these macros
#define SK_ENV            (sk_thread_env())
#define SK_ENV_CORE       (sk_thread_env()->core)

#define SK_ENV_WORKFLOWS  (sk_thread_env()->core->workflows)
#define SK_ENV_LOGGER     (sk_thread_env()->core->logger)
#define SK_ENV_SCHED      (sk_thread_env()->engine->sched)
#define SK_ENV_ENTITY_MGR (sk_thread_env()->engine->entity_mgr)
#define SK_ENV_EVENTLOOP  (sk_thread_env()->engine->evlp)
#define SK_ENV_MON        (sk_thread_env()->engine->mon)

#define SK_ENV_NAME_LEN 20

typedef struct sk_thread_env_t {
    // ======== public  ========
    sk_core_t*   core;
    sk_engine_t* engine;

    // used for logging or debugging
    char name[SK_ENV_NAME_LEN];
    int  idx;
} sk_thread_env_t;

void sk_thread_env_init();
void sk_thread_env_set(sk_thread_env_t* env);
sk_thread_env_t* sk_thread_env();
sk_thread_env_t* sk_thread_env_create(sk_core_t* core, sk_engine_t* engine,
                                      const char* name, int idx);

#endif

