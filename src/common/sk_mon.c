#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "fhash/fhash.h"
#include "api/sk_utils.h"
#include "api/sk_env.h"
#include "api/sk_mon.h"

struct sk_mon_t {
    fhash*          mon_tbl;
    pthread_mutex_t lock;
};

static
int _hash_str_compare(const void* key1, key_sz_t key_sz1,
                      const void* key2, key_sz_t key_sz2)
{
    if (key_sz1 != key_sz2) {
        return 1;
    }

    const char* skey1 = (const char*)key1;
    const char* skey2 = (const char*)key2;
    return strncmp(skey1, skey2, (size_t)key_sz1);
}

sk_mon_t* sk_mon_create()
{
    sk_mon_t* sk_mon = calloc(1, sizeof(*sk_mon));

    fhash_opt opt;
    opt.hash_alg = NULL;
    opt.compare = _hash_str_compare;
    sk_mon->mon_tbl = fhash_create(0, opt, FHASH_MASK_AUTO_REHASH);
    SK_ASSERT(sk_mon->mon_tbl);

    int ret = pthread_mutex_init(&sk_mon->lock, NULL);
    SK_ASSERT(ret == 0);

    return sk_mon;
}

void sk_mon_destroy(sk_mon_t* sk_mon)
{
    if (!sk_mon) {
        return;
    }

    fhash_delete(sk_mon->mon_tbl);
    int ret = pthread_mutex_destroy(&sk_mon->lock);
    SK_ASSERT(ret == 0);
    free(sk_mon);
}

void sk_mon_inc(sk_mon_t* sk_mon, const char* name, uint32_t value)
{
    SK_ASSERT(name);
    key_sz_t name_len = (key_sz_t)strlen(name);
    SK_ASSERT(name_len);

    pthread_mutex_lock(&sk_mon->lock);
    {
        uint32_t* raw = fhash_get(sk_mon->mon_tbl, name, name_len, NULL);
        uint32_t raw_value = raw ? *raw : 0;

        uint32_t new_value = raw_value + value;
        fhash_set(sk_mon->mon_tbl, name, name_len,
                  &new_value, sizeof(new_value));

        sk_print("metrics inc: %s - %u, thread name: %s\n",
                 name, value, SK_THREAD_ENV->name);
    }
    pthread_mutex_unlock(&sk_mon->lock);
}

uint32_t sk_mon_get(sk_mon_t* sk_mon, const char* name)
{
    SK_ASSERT(name);
    key_sz_t name_len = (key_sz_t)strlen(name);
    SK_ASSERT(name_len);

    uint32_t raw_value = 0;
    pthread_mutex_lock(&sk_mon->lock);
    {
        uint32_t* raw = fhash_get(sk_mon->mon_tbl, name, name_len, NULL);
        raw_value = raw ? *raw : 0;
    }
    pthread_mutex_unlock(&sk_mon->lock);

    return raw_value;
}

void sk_mon_reset(sk_mon_t* sk_mon)
{
    pthread_mutex_lock(&sk_mon->lock);
    {
        fhash_iter iter = fhash_iter_new(sk_mon->mon_tbl);
        void* value = NULL;

        while((value = fhash_next(&iter))) {
            uint32_t value = 0;
            fhash_set(sk_mon->mon_tbl, iter.key, iter.key_sz,
                      &value, sizeof(value));
        }
        fhash_iter_release(&iter);
    }
    pthread_mutex_unlock(&sk_mon->lock);
}

// NOTES: This api may involve the *DEAD LOCKING*, should remove this api in the
// future
void sk_mon_foreach(sk_mon_t* sk_mon, sk_mon_cb cb, void* ud)
{
    pthread_mutex_lock(&sk_mon->lock);
    {
        sk_print("sk_mon_foreach: mon=%p\n", (void*)sk_mon);
        fhash_iter iter = fhash_iter_new(sk_mon->mon_tbl);
        void* value = NULL;

        while((value = fhash_next(&iter))) {
            sk_print("metrics cb: %s - %u, thread name: %s\n",
                     iter.key, *(uint32_t*)value, SK_THREAD_ENV->name);

            cb(iter.key, *(uint32_t*)value, ud);
        }
        fhash_iter_release(&iter);
    }
    pthread_mutex_unlock(&sk_mon->lock);
}
