#ifndef SKULL_METRICS_UTILS_H
#define SKULL_METRICS_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

void skull_metric_inc(const char* name, double value);
double skull_metric_get(const char* name);

#ifdef __cplusplus
}
#endif

#endif

