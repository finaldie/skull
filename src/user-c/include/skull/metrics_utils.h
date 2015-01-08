#ifndef SKULL_METRICS_UTILS_H
#define SKULL_METRICS_UTILS_H

void skull_metric_inc(const char* name, double value);
double skull_metric_get(const char* name);

typedef void (*skull_metric_each)(const char* name, double value, void* ud);
void skull_metric_foreach(skull_metric_each, void* ud);

#endif

