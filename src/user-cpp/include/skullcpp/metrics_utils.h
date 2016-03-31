#ifndef SKULLCPP_METRICS_UTILS_H
#define SKULLCPP_METRICS_UTILS_H

namespace skullcpp {

void metricInc(const char* name, double value);
double metricGet(const char* name);

} // End of namespace

#endif

