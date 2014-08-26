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
