#ifndef SK_CONFIG_H
#define SK_CONFIG_H

#include "flist/flist.h"
#include "api/sk_module.h"

#define SK_CONFIG_LOCATION_LEN    1024

#define SK_WORKFLOW_NONCONCURRENT 0
#define SK_WORKFLOW_CONCURRENT    1

// the type of workflow
#define SK_WORKFLOW_MAIN    0
#define SK_WORKFLOW_NETWORK 1


typedef void (*sk_load_mapping)(int depth, const char* key, const char* value,
                                void* data);
typedef void (*sk_load_sequence)(int depth, const char* key, const char* value,
                                 void* data);
typedef void (*sk_load_array_item)(int depth, const char* key,
                                   const char* value, void* data);

typedef struct sk_workflow_cfg_t {
    int type;
    int concurrent;

    flist* modules; // store module names
} sk_workflow_cfg_t;

typedef struct sk_config_t {
    char location[SK_CONFIG_LOCATION_LEN]; // the location of the config file

    int threads;
    int _reserved;
    flist* workflows;
} sk_config_t;

void sk_load_config(sk_config_t* config);

#endif

