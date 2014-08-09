#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "api/sk_utils.h"
#include "api/sk_config_loader.h"
#include "api/sk_config.h"

static
sk_workflow_cfg_t* _create_workflow_cfg()
{
    sk_workflow_cfg_t* workflow = calloc(1, sizeof(*workflow));
    workflow->modules = flist_create();
    return workflow;
}

static
void _delete_workflow_cfg(sk_workflow_cfg_t* workflow)
{
    char* module_name = NULL;
    while ((module_name = flist_pop(workflow->modules))) {
        free(module_name);
    }
    flist_delete(workflow->modules);
    free(workflow);
}

static
sk_config_t* _create_config()
{
    sk_config_t* config = calloc(1, sizeof(*config));
    config->workflows = flist_create();
    return config;
}

static
void _delete_config(sk_config_t* config)
{
    sk_workflow_cfg_t* workflow = NULL;

    while ((workflow = flist_pop(config->workflows))) {
        _delete_workflow_cfg(workflow);
    }
    flist_delete(config->workflows);
    free(config);
}

static
int _get_int(sk_cfg_node_t* node)
{
    int value = strtol(node->data.value, NULL, 10);
    SK_ASSERT_MSG(errno != EINVAL && errno != ERANGE,
                  "load config errno: %d, %s\n", errno, strerror(errno));

    return value;
}

static
void _load_modules(sk_cfg_node_t* node, sk_workflow_cfg_t* workflow)
{
    SK_ASSERT_MSG(node->type == SK_CFG_NODE_ARRAY,
                  "workflow:modules must be a sequence\n");

    sk_cfg_node_t* child = NULL;
    flist_iter iter = flist_new_iter(node->data.array);
    while ((child = flist_each(&iter))) {
        SK_ASSERT(child->type == SK_CFG_NODE_VALUE);

        int ret = flist_push(workflow->modules, strdup(child->data.value));
        SK_ASSERT(!ret);
    }
}

// example yaml config:
// workflow:
//     - modules: [test]
//       concurrent: 1
//       port: 7758
static
void _load_workflow(sk_cfg_node_t* node, sk_config_t* config)
{
    SK_ASSERT_MSG(node->type == SK_CFG_NODE_ARRAY,
                  "workflow config must be a sequence\n");

    sk_cfg_node_t* child = NULL;
    flist_iter iter = flist_new_iter(node->data.array);
    while ((child = flist_each(&iter))) {
        SK_ASSERT_MSG(child->type == SK_CFG_NODE_MAPPING,
                      "workflow sequence item must be a mapping\n");

        sk_workflow_cfg_t* workflow = _create_workflow_cfg();

        // load modules and concurrent
        fhash_str_iter item_iter = fhash_str_iter_new(child->data.mapping);
        while ((child = fhash_str_next(&item_iter))) {
            const char* key = item_iter.key;

            if (0 == strcmp(key, "modules")) {
                _load_modules(child, workflow);
            } else if (0 == strcmp(key, "concurrent")) {
                workflow->concurrent = _get_int(child);
            } else if (0 == strcmp(key, "port")) {
                workflow->port = _get_int(child);
            }
        }
        fhash_str_iter_release(&item_iter);

        int ret = flist_push(config->workflows, workflow);
        SK_ASSERT(!ret);
    }
}

static
void _load_config(sk_cfg_node_t* root, sk_config_t* config)
{
    SK_ASSERT_MSG(root->type == SK_CFG_NODE_MAPPING,
                  "root node type: %d\n", root->type);

    fhash_str_iter iter = fhash_str_iter_new(root->data.mapping);
    sk_cfg_node_t* child = NULL;

    while ((child = fhash_str_next(&iter))) {
        // load thread_num
        const char* key = iter.key;
        if (0 == strcmp(key, "thread_num")) {
            config->threads = _get_int(child);
        }

        // load working flows
        if (0 == strcmp(key, "workflows")) {
            _load_workflow(child, config);
        }
    }

    fhash_str_iter_release(&iter);
}

sk_config_t* sk_config_create(const char* filename)
{
    // create sk_config
    sk_config_t* config = _create_config();
    strncpy(config->location, filename, SK_CONFIG_LOCATION_LEN);
    const char* location = config->location;

    sk_cfg_node_t* root = sk_config_load(location);
    sk_config_dump(root);

    _load_config(root, config);
    sk_config_delete(root);
    return config;
}

void sk_config_destroy(sk_config_t* config)
{
    _delete_config(config);
}

void sk_config_print(sk_config_t* config)
{
    sk_print("dump sk_config:\n");
    sk_print("thread_num: %d\n", config->threads);

    sk_workflow_cfg_t* workflow = NULL;
    flist_iter iter = flist_new_iter(config->workflows);
    while ((workflow = flist_each(&iter))) {
        sk_print("workflow start:\n");
        sk_print("\tconcurrent: %d\n", workflow->concurrent);
        sk_print("\tmodules: ");

        char* module_name = NULL;
        flist_iter name_iter = flist_new_iter(workflow->modules);
        while ((module_name = flist_each(&name_iter))) {
            sk_rawprint("%s ", module_name);
        }

        sk_print("\n");
        sk_print("workflow end:\n");
    }
}
