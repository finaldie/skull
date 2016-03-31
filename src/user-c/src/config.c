#include <stdlib.h>
#include <string.h>

#include "api/sk_utils.h"
#include "api/sk_config_loader.h"

#include "skull/config.h"

struct skull_config_t {
    const char*    config_file_name;
    sk_cfg_node_t* config;
};

skull_config_t* skull_config_create(const char* config_file_name)
{
    if (!config_file_name) {
        sk_print("error: config file name is NULL\n");
        return NULL;
    }

    sk_cfg_node_t* cfg = sk_config_load(config_file_name);
    if (!cfg) {
        sk_print("error: config %s load failed\n", config_file_name);
        return NULL;
    }

    skull_config_t* config = calloc(1, sizeof(*config));
    config->config_file_name = strdup(config_file_name);
    config->config = cfg;
    return config;
}

void skull_config_destroy(const skull_config_t* config)
{
    if (!config) {
        return;
    }

    free((void*)config->config_file_name);
    sk_config_delete(config->config);
    free((void*)config);
}

int    skull_config_getint    (const skull_config_t* config, const char* key_name,
                               int default_value)
{
    sk_cfg_node_t* root = config->config;
    SK_ASSERT_MSG(root->type == SK_CFG_NODE_MAPPING,
                  "skull config root node type: %d\n", root->type);

    if (!key_name) {
        return 0;
    }

    int value = default_value;
    sk_cfg_node_t* target_node = fhash_str_get(root->data.mapping, key_name);
    if (target_node) {
        value = sk_config_getint(target_node);
    }

    return value;
}

double skull_config_getdouble (const skull_config_t* config, const char* key_name,
                               double default_value)
{
    sk_cfg_node_t* root = config->config;
    SK_ASSERT_MSG(root->type == SK_CFG_NODE_MAPPING,
                  "skull config root node type: %d\n", root->type);

    if (!key_name) {
        return 0;
    }

    double value = default_value;
    sk_cfg_node_t* target_node = fhash_str_get(root->data.mapping, key_name);
    if (target_node) {
        value = sk_config_getdouble(target_node);
    }

    return value;
}

const char* skull_config_getstring(const skull_config_t* config,
                                   const char* key_name)
{
    sk_cfg_node_t* root = config->config;
    SK_ASSERT_MSG(root->type == SK_CFG_NODE_MAPPING,
                  "skull config root node type: %d\n", root->type);

    if (!key_name) {
        return 0;
    }

    const char* value = "";
    sk_cfg_node_t* target_node = fhash_str_get(root->data.mapping, key_name);
    if (target_node) {
        value = sk_config_getstring(target_node);
    }

    return value;
}
