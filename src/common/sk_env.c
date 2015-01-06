#include <stdlib.h>
#include <pthread.h>

#include "api/sk_utils.h"
#include "api/sk_env.h"

static pthread_key_t sk_env_key;

static
void _sk_thread_exit(void* data)
{
    sk_print("thread exit\n");
    free(data);
}


// APIs

void sk_thread_env_init()
{
    pthread_key_create(&sk_env_key, _sk_thread_exit);
}

void sk_thread_env_set(sk_thread_env_t* env)
{
    pthread_setspecific(sk_env_key, env);
}

sk_thread_env_t* sk_thread_env()
{
    return pthread_getspecific(sk_env_key);
}

sk_thread_env_t* sk_thread_env_create(skull_core_t* core,
                                      sk_engine_t* engine,
                                      const char* name,
                                      int idx)
{
    sk_thread_env_t* thread_env = calloc(1, sizeof(*thread_env));
    thread_env->core = core;
    thread_env->engine = engine;

    snprintf(thread_env->name, SK_ENV_NAME_LEN, "%s", name);
    thread_env->idx = idx;

    return thread_env;
}
