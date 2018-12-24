#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "flibs/fhash.h"
#include "api/sk_utils.h"
#include "api/sk_env.h"
#include "api/sk_mon.h"

struct sk_mon_snapshot_t {
    time_t start;
    time_t end;

    fhash* map;
};

struct sk_mon_t {
    fhash*          mon_tbl;
    pthread_mutex_t lock;
    time_t          start;

    // Window size at least >= 2
    sk_mon_snapshot_t* prev[SK_MON_DEFAULT_WINDOW];
};

static sk_mon_snapshot_t* _sk_mon_snapshot_create(time_t start, time_t end);

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

static
fhash_opt _sk_mon_hash_opt = {
    .hash_alg = NULL,
    .compare  = _hash_str_compare
};

sk_mon_t* sk_mon_create()
{
    sk_mon_t* sk_mon = calloc(1, sizeof(*sk_mon));

    sk_mon->mon_tbl = fhash_create(SK_MON_DEFAULT_SIZE, _sk_mon_hash_opt,
                                   FHASH_MASK_AUTO_REHASH);
    SK_ASSERT(sk_mon->mon_tbl);

    int ret = pthread_mutex_init(&sk_mon->lock, NULL);
    SK_ASSERT(ret == 0);

    sk_mon->start = time(NULL);

    return sk_mon;
}

void sk_mon_destroy(sk_mon_t* sk_mon)
{
    if (!sk_mon) {
        return;
    }

    fhash_delete(sk_mon->mon_tbl);
    for (int i = 0; i < SK_MON_DEFAULT_WINDOW; i++) {
        sk_mon_snapshot_destroy(sk_mon->prev[i]);
    }
    int ret = pthread_mutex_destroy(&sk_mon->lock);
    SK_ASSERT(ret == 0);
    free(sk_mon);
}

void sk_mon_inc(sk_mon_t* sk_mon, const char* name, double value)
{
    SK_ASSERT(name);
    key_sz_t name_len = (key_sz_t)strlen(name);
    SK_ASSERT(name_len);

    pthread_mutex_lock(&sk_mon->lock);
    {
        double* raw = fhash_get(sk_mon->mon_tbl, name, name_len, NULL);
        double raw_value = raw ? *raw : 0.0f;

        double new_value = raw_value + value;
        fhash_set(sk_mon->mon_tbl, name, name_len,
                  &new_value, sizeof(new_value));

        //sk_print("metrics inc: %s - %f, thread name: %s\n",
        //         name, value, SK_ENV->name);
    }
    pthread_mutex_unlock(&sk_mon->lock);
}

void sk_mon_set(sk_mon_t* sk_mon, const char* name, double value)
{
    SK_ASSERT(name);
    key_sz_t name_len = (key_sz_t)strlen(name);
    SK_ASSERT(name_len);

    pthread_mutex_lock(&sk_mon->lock);
    {
        fhash_set(sk_mon->mon_tbl, name, name_len,
                  &value, sizeof(value));

        //sk_print("metrics set: %s - %f, thread name: %s\n",
        //         name, value, SK_ENV->name);
    }
    pthread_mutex_unlock(&sk_mon->lock);
}

double sk_mon_get(sk_mon_t* sk_mon, const char* name)
{
    SK_ASSERT(name);
    key_sz_t name_len = (key_sz_t)strlen(name);
    SK_ASSERT(name_len);

    double raw_value = 0.0f;
    pthread_mutex_lock(&sk_mon->lock);
    {
        double* raw = fhash_get(sk_mon->mon_tbl, name, name_len, NULL);
        raw_value = raw ? *raw : 0.0f;
    }
    pthread_mutex_unlock(&sk_mon->lock);

    return raw_value;
}

void sk_mon_snapshot_and_reset(sk_mon_t* sk_mon)
{
    pthread_mutex_lock(&sk_mon->lock);
    {
        // 1. Create snapshot
        time_t now = time(NULL);
        sk_mon_snapshot_t* snapshot =
            _sk_mon_snapshot_create(sk_mon->start, now);

        // 2. Reset data and fill into snapshot
        sk_mon->start = now;

        fhash_iter iter = fhash_iter_new(sk_mon->mon_tbl);
        void* value = NULL;

        while((value = fhash_next(&iter))) {
            // 2.1 Fill old data into snapshot
            fhash_set(snapshot->map, iter.key, iter.key_sz,
                      iter.value, iter.value_sz);

            // 2.2 Reset data
            double value = 0.0f;
            fhash_set(sk_mon->mon_tbl, iter.key, iter.key_sz,
                      &value, sizeof(value));
        }
        fhash_iter_release(&iter);

        // 3. Insert new one in front and release oldest snapshot
        for (int i = 0; i < SK_MON_DEFAULT_WINDOW; i++) {
            sk_mon_snapshot_t* tmp = sk_mon->prev[i];
            sk_mon->prev[i] = snapshot;
            snapshot = tmp;
        }

        // Now, the snapshot is pointed to oldest one
        sk_mon_snapshot_destroy(snapshot);
    }
    pthread_mutex_unlock(&sk_mon->lock);

    sk_print("snapshot one done\n");
}

static
sk_mon_snapshot_t* sk_mon_snapshot_prev(sk_mon_t* sk_mon, int loc)
{
    if (loc < 0 || loc >= SK_MON_DEFAULT_WINDOW) return NULL;
    return sk_mon->prev[loc];
}

sk_mon_snapshot_t* sk_mon_snapshot(sk_mon_t* sk_mon, int loc)
{
    sk_mon_snapshot_t* snapshot = NULL;

    pthread_mutex_lock(&sk_mon->lock);
    {
        if (loc) {
            snapshot = sk_mon_snapshot_prev(sk_mon, loc - 1);
            pthread_mutex_unlock(&sk_mon->lock);
            return snapshot;
        }

        snapshot = _sk_mon_snapshot_create(sk_mon->start, time(NULL));

        fhash_iter iter = fhash_iter_new(sk_mon->mon_tbl);
        void* value = NULL;

        while((value = fhash_next(&iter))) {
            fhash_set(snapshot->map, iter.key, iter.key_sz,
                      iter.value, iter.value_sz);
        }
        fhash_iter_release(&iter);
    }
    pthread_mutex_unlock(&sk_mon->lock);

    return snapshot;
}

void sk_mon_snapshot_all(sk_core_t* core)
{
    // 1. snapshot global metrics
    sk_mon_snapshot_and_reset(core->mon);

    // 2. snapshot master metrics
    sk_mon_snapshot_and_reset(core->master->mon);

    // 3. snapshot workers metrics
    int threads = core->config->threads;
    for (int i = 0; i < threads; i++) {
        sk_engine_t* worker = core->workers[i];
        sk_mon_snapshot_and_reset(worker->mon);
    }

    // 4. snaphost bio metrics
    for (int i = 0; i < core->config->bio_cnt; i++) {
        sk_engine_t* bio = core->bio[i];
        sk_mon_snapshot_and_reset(bio->mon);
    }

    // 5. shapshot user metrics
    sk_mon_snapshot_and_reset(core->umon);

    sk_print("snapshot all done\n");
}

/**********************************mon snapshot********************************/
static
sk_mon_snapshot_t* _sk_mon_snapshot_create(time_t start, time_t end)
{
    sk_mon_snapshot_t* snapshot = calloc(1, sizeof(*snapshot));
    snapshot->start = start;
    snapshot->end   = end;
    snapshot->map =
        fhash_create(SK_MON_DEFAULT_SIZE, _sk_mon_hash_opt, FHASH_MASK_AUTO_REHASH);

    return snapshot;
}

void sk_mon_snapshot_destroy(sk_mon_snapshot_t* snapshot)
{
    if (!snapshot) return;

    fhash_delete(snapshot->map);
    free(snapshot);
}

time_t sk_mon_snapshot_starttime(sk_mon_snapshot_t* snapshot)
{
    return snapshot->start;
}

time_t sk_mon_snapshot_endtime(sk_mon_snapshot_t* snapshot)
{
    return snapshot->end;
}

void sk_mon_snapshot_foreach(sk_mon_snapshot_t* ss, sk_mon_cb cb, void* ud)
{
    fhash_iter iter = fhash_iter_new(ss->map);
    void* value = NULL;

    while((value = fhash_next(&iter))) {
        sk_print("metrics cb: %s - %f, thread name: %s\n",
                 (const char*)iter.key, *(double*)value,
                 SK_ENV->name);

        cb(iter.key, *(double*)value, ud);
    }
    fhash_iter_release(&iter);
}
