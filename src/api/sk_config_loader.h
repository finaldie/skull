#ifndef SK_CONFIG_LOADER_H
#define SK_CONFIG_LOADER_H

#include "fhash/fhash.h"
#include "flist/flist.h"

typedef enum sk_cfg_node_type_t {
    SK_CFG_NODE_VALUE = 0,   // a value of integer of string...
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
        flist* array;

        // The SK_CFG_NODE_MAPPING paratmer
        fhash* mapping;
    } data;
} sk_cfg_node_t;

// return the root node of a yaml file
sk_cfg_node_t* sk_config_load(const char* filename);

void sk_config_delete(sk_cfg_node_t* root);

void sk_config_dump(sk_cfg_node_t* root);



#endif

