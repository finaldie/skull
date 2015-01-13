#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "flibs/flog.h"

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
    long int value = strtol(node->data.value, NULL, 10);
    SK_ASSERT_MSG(errno != EINVAL && errno != ERANGE,
                  "load config errno: %d, %s\n", errno, strerror(errno));

    return (int)value;
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
                workflow->concurrent = sk_config_getint(child);
            } else if (0 == strcmp(key, "port")) {
                int port = sk_config_getint(child);
                SK_ASSERT_MSG(port > 0 && port <= 65535, "port[%d] should be "
                              "in (0, 65535]\n", port);

                workflow->port = (in_port_t)port;
            }
        }
        fhash_str_iter_release(&item_iter);

        int ret = flist_push(config->workflows, workflow);
        SK_ASSERT(!ret);
    }
}

static
void _load_log_name(sk_cfg_node_t* child, sk_config_t* config)
{
    SK_ASSERT(child->type == SK_CFG_NODE_VALUE);
    const char* log_name = child->data.value;

    if (log_name && strlen(log_name)) {
        strncpy(config->log_name, child->data.value, SK_CONFIG_LOGNAME_LEN - 1);
    } else {
        sk_print_err("Fatal: empty log name, please configure a non-empty \
                     log name\n");
        exit(1);
    }
}

static
void _load_log_level(sk_cfg_node_t* child, sk_config_t* config)
{
    SK_ASSERT(child->type == SK_CFG_NODE_VALUE);
    const char* log_level_str = child->data.value;
    if (0 == strcmp("trace", log_level_str)) {
        config->log_level = FLOG_LEVEL_TRACE;
    } else if (0 == strcmp("debug", log_level_str)) {
        config->log_level = FLOG_LEVEL_DEBUG;
    } else if (0 == strcmp("info", log_level_str)) {
        config->log_level = FLOG_LEVEL_INFO;
    } else if (0 == strcmp("warn", log_level_str)) {
        config->log_level = FLOG_LEVEL_WARN;
    } else if (0 == strcmp("error", log_level_str)) {
        config->log_level = FLOG_LEVEL_ERROR;
    } else if (0 == strcmp("fatal", log_level_str)) {
        config->log_level = FLOG_LEVEL_FATAL;
    } else {
        sk_print_err("Fatal: unknown log level: %s\n", log_level_str);
        exit(1);
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
            config->threads = sk_config_getint(child);
        }

        // load working flows
        if (0 == strcmp(key, "workflows")) {
            _load_workflow(child, config);
        }

        // load log name
        if (0 == strcmp(key, "log_name")) {
            _load_log_name(child, config);
        }

        // load log level: trace|debug|info|warn|error|fatal
        if (0 == strcmp(key, "log_level")) {
            _load_log_level(child, config);
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

    // pick useful information to skull_config struct
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
    sk_print("log_name: %s\n", config->log_name);
    sk_print("log_level: %d\n", config->log_level);

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

        sk_rawprint("\n");
        sk_print("workflow end:\n");
    }
}
