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
