#ifndef SK_CONFIG_LOADER_H
#define SK_CONFIG_LOADER_H

#include "flibs/fhash.h"
#include "flibs/flist.h"

typedef enum sk_cfg_node_type_t {
    SK_CFG_NODE_VALUE = 0,   // a value of integer or string...
    SK_CFG_NODE_ARRAY = 1,   // sequence
    SK_CFG_NODE_MAPPING = 2  // mapping
} sk_cfg_node_type_t;

typedef struct sk_cfg_node_t {
    sk_cfg_node_type_t type;
    int size;

    union {
        // The SK_CFG_NODE_VALUE parameter
        char* value;

        // The SK_CFG_NODE_ARRAY parameter
        // value: sk_cfg_node_t
        flist* array;

        // The SK_CFG_NODE_MAPPING paratmer
        // key: string, value: sk_cfg_node_t
        fhash* mapping;
    } data;
} sk_cfg_node_t;

// return the root node of a yaml file
sk_cfg_node_t* sk_config_load(const char* filename);

void sk_config_delete(sk_cfg_node_t* root);

void sk_config_dump(sk_cfg_node_t* root);

// Util APIs
int sk_config_getint(sk_cfg_node_t* node);
double sk_config_getdouble(sk_cfg_node_t* node);
const char* sk_config_getstring(sk_cfg_node_t* node);

#endif

