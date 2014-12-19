#include <stdlib.h>

#include <api/sk_mon.h>
#include <api/sk_env.h>

#include <skull/metrics_utils.h>

void skull_metric_inc(const char* name, uint32_t value)
{
    sk_mon_t* mon = SK_THREAD_ENV->core->mon;
    sk_mon_inc(mon, name, value);
}

uint32_t skull_metric_get(const char* name)
{
    sk_mon_t* mon = SK_THREAD_ENV->core->mon;
    return sk_mon_get(mon, name);
}
