#ifndef SKULL_METRICS_UTILS_H
#define SKULL_METRICS_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

void skull_metric_inc(const char* name, uint32_t value);
uint32_t skull_metric_get(const char* name);

typedef void (*skull_metric_each)(const char* name, uint32_t value, void* ud);
void skull_metric_foreach(skull_metric_each, void* ud);

#ifdef __cplusplus
}
#endif

#endif

