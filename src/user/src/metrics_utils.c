#include <stdlib.h>
#include <stdio.h>

#include <api/sk_mon.h>
#include <api/sk_env.h>

#include <skull/metrics_utils.h>

void skull_metric_inc(const char* name, double value)
{
    sk_mon_t* mon = SK_ENV->core->umon;
    sk_mon_inc(mon, name, value);
}

double skull_metric_get(const char* name)
{
    sk_mon_t* mon = SK_ENV->core->umon;
    return sk_mon_get(mon, name);
}

void skull_metric_foreach(skull_metric_each metric_cb, void* ud)
{
    skull_core_t* core = SK_ENV->core;

    // iterate global metrics
    sk_mon_foreach(core->mon, metric_cb, ud);

    // iterate master metrics
    skull_sched_t* master = &core->main_sched;
    sk_mon_foreach(master->mon, metric_cb, ud);

    // iterate worker metrics
    int threads = core->config->threads;
    for (int i = 0; i < threads; i++) {
        skull_sched_t* worker = &core->worker_sched[i];
        sk_mon_foreach(worker->mon, metric_cb, ud);
    }

    // iterate user metrics
    sk_mon_foreach(core->umon, metric_cb, ud);
}
