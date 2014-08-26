#ifndef SK_ENV_H
#define SK_ENV_H

#include "flist/flist.h"
#include "api/sk_entity.h"
#include "api/sk_entity_mgr.h"
#include "api/sk_sched.h"

#define SK_THREAD_ENV_SCHED      (sk_thread_env()->sched)
#define SK_THREAD_ENV_ENTITY_MGR (sk_thread_env()->entity_mgr)
#define SK_THREAD_ENV_WORKFLOWS  (sk_thread_env()->workflows)
#define SK_THREAD_ENV_EVENTLOOP  (sk_thread_env()->evlp)

typedef struct sk_thread_env_t {
    sk_sched_t*      sched;
    sk_entity_mgr_t* entity_mgr;
    flist*           workflows;
    void*            evlp;
} sk_thread_env_t;

void sk_thread_env_init();
void sk_thread_env_set(sk_thread_env_t* env);
sk_thread_env_t* sk_thread_env();

#endif

