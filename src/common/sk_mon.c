#include <stdlib.h>
#include <pthread.h>

#include "fhash/fhash_str.h"
#include "api/sk_utils.h"
#include "api/sk_mon.h"

struct sk_mon_t {
    fhash*          mon_tbl;
    pthread_mutex_t lock;
};

sk_mon_t* sk_mon_create()
{
    sk_mon_t* sk_mon = calloc(1, sizeof(*sk_mon));
    sk_mon->mon_tbl = fhash_str_create(0, FHASH_MASK_AUTO_REHASH);
    int ret = pthread_mutex_init(&sk_mon->lock, NULL);
    SK_ASSERT(ret == 0);

    return sk_mon;
}

void sk_mon_destroy(sk_mon_t* sk_mon)
{
    if (!sk_mon) {
        return;
    }

    fhash_str_delete(sk_mon->mon_tbl);
    int ret = pthread_mutex_destroy(&sk_mon->lock);
    SK_ASSERT(ret == 0);
    free(sk_mon);
}

void sk_mon_inc(sk_mon_t* sk_mon, const char* name, uint32_t value)
{
    pthread_mutex_lock(&sk_mon->lock);
    {
        uint32_t raw_value = (uint32_t)(uintptr_t)fhash_str_get(sk_mon->mon_tbl,
                                                                name);
        uint32_t new_value = raw_value + value;
        fhash_str_set(sk_mon->mon_tbl, name, (void*)(uintptr_t)new_value);
    }
    pthread_mutex_unlock(&sk_mon->lock);
}

uint32_t sk_mon_get(sk_mon_t* sk_mon, const char* name)
{
    uint32_t raw_value = 0;
    pthread_mutex_lock(&sk_mon->lock);
    {
        raw_value = (uint32_t)(uintptr_t)fhash_str_get(sk_mon->mon_tbl, name);
    }
    pthread_mutex_unlock(&sk_mon->lock);

    return raw_value;
}

void sk_mon_reset(sk_mon_t* sk_mon)
{
    pthread_mutex_lock(&sk_mon->lock);
    {
        fhash_str_iter iter = fhash_str_iter_new(sk_mon->mon_tbl);
        void* value = NULL;

        while((value = fhash_str_next(&iter))) {
            fhash_str_set(sk_mon->mon_tbl, iter.key, (void*)0);
        }
        fhash_str_iter_release(&iter);
    }
    pthread_mutex_unlock(&sk_mon->lock);
}

void sk_mon_foreach(sk_mon_t* sk_mon, sk_mon_cb cb, void* ud)
{
    pthread_mutex_lock(&sk_mon->lock);
    {
        fhash_str_iter iter = fhash_str_iter_new(sk_mon->mon_tbl);
        void* value = NULL;

        while((value = fhash_str_next(&iter))) {
            cb(iter.key, (uint32_t)(uintptr_t)value, ud);
        }
        fhash_str_iter_release(&iter);
    }
    pthread_mutex_unlock(&sk_mon->lock);
}
