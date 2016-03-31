#include <stdlib.h>
#include <stdio.h>

#include <skull/metrics_utils.h>
#include <skullcpp/metrics_utils.h>

namespace skullcpp {

void metricInc(const char* name, double value)
{
    skull_metric_inc(name, value);
}

double metricGet(const char* name)
{
    return skull_metric_get(name);
}

} // End of namespace
