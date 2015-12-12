#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "flibs/flog.h"

#include "api/sk_utils.h"
#include "api/sk_env.h"
#include "api/sk_config_loader.h"
#include "api/sk_config.h"

static
sk_workflow_cfg_t* _create_workflow_cfg()
{
    sk_workflow_cfg_t* workflow = calloc(1, sizeof(*workflow));
    workflow->port = SK_CONFIG_NO_PORT;
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

    free((void*)workflow->idl_name);
    free(workflow);
}

static
sk_service_cfg_t* _service_cfg_item_create()
{
    sk_service_cfg_t* cfg = calloc(1, sizeof(*cfg));
    cfg->enable = false;
    cfg->data_mode = SK_SRV_DATA_MODE_RW_PR;
    cfg->apis = fhash_str_create(0, FHASH_MASK_AUTO_REHASH);

    return cfg;
}

static
void _service_cfg_item_destroy(sk_service_cfg_t* cfg)
{
    if (!cfg) {
        return;
    }

    // destroy api items
    fhash_str_iter api_iter = fhash_str_iter_new(cfg->apis);
    sk_srv_api_cfg_t* api_cfg_item = NULL;

    while ((api_cfg_item = fhash_str_next(&api_iter))) {
        free(api_cfg_item);
    }

    fhash_str_iter_release(&api_iter);

    fhash_str_delete(cfg->apis);
    free(cfg);
}

static
sk_config_t* _create_config()
{
    sk_config_t* config = calloc(1, sizeof(*config));
    config->workflows = flist_create();
    config->services = fhash_str_create(0, FHASH_MASK_AUTO_REHASH);
    return config;
}

static
void _delete_config(sk_config_t* config)
{
    sk_workflow_cfg_t* workflow = NULL;

    // destroy workflow
    while ((workflow = flist_pop(config->workflows))) {
        _delete_workflow_cfg(workflow);
    }
    flist_delete(config->workflows);

    // destroy service
    fhash_str_iter srv_iter = fhash_str_iter_new(config->services);
    sk_service_cfg_t* srv_cfg_item = NULL;

    while ((srv_cfg_item = fhash_str_next(&srv_iter))) {
        _service_cfg_item_destroy(srv_cfg_item);
    }

    fhash_str_iter_release(&srv_iter);
    fhash_str_delete(config->services);

    free(config);
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

    int enabled_stdin = 0;
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
            } else if (0 == strcmp(key, "idl")) {
                workflow->idl_name = strdup(child->data.value);
            } else if (0 == strcmp(key, "concurrent")) {
                workflow->concurrent = (uint32_t) sk_config_getint(child) & 0x1;
            } else if (0 == strcmp(key, "port")) {
                int port = sk_config_getint(child);
                if (port <= 0) {
                    sk_print("port (%d) is <= 0, skip it\n", port);
                    continue;
                }

                SK_ASSERT_MSG(port > 0 && port <= 65535, "port[%d] should be "
                              "in (0, 65535]\n", port);

                workflow->port = port;
            } else if (0 == strcmp(key, "stdin")) {
                workflow->enable_stdin =
                    (uint32_t) sk_config_getint(child) & 0x1;

                SK_ASSERT_MSG(enabled_stdin == 0,
                              "Only one workflow can enable stdin\n");
                enabled_stdin = 1;
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
        sk_print_err("Fatal: empty log name, please configure a non-empty "
                     "log name\n");
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
void _load_service_data_mode(const sk_cfg_node_t* child, sk_service_cfg_t* cfg)
{
    SK_ASSERT(child->type == SK_CFG_NODE_VALUE);

    const char* data_mode = child->data.value;

    if (data_mode) {
        if (0 == strcmp(data_mode, "rw-pr")) {
            cfg->data_mode = SK_SRV_DATA_MODE_RW_PR;
        } else if (0 == strcmp(data_mode, "rw-pw")) {
            cfg->data_mode = SK_SRV_DATA_MODE_RW_PW;
        } else {
            sk_print_err("Fatal: unknown service data mode '%s', use "
                         "rw-pr or rw-pw\n", data_mode);
            exit(1);
        }
    } else {
        sk_print_err("Fatal: empty service data mode, please configure a "
                     "non-empty data mode\n");
        exit(1);
    }
}

static
void _load_service_api(const sk_cfg_node_t* node, const char* service_name,
                       const char* api_name, sk_service_cfg_t* cfg)
{
    SK_ASSERT_MSG(node->type == SK_CFG_NODE_MAPPING,
                  "service %s api item config must be a mapping\n",
                  service_name);

    // Create a api cfg structure
    sk_srv_api_cfg_t* api_cfg = calloc(1, sizeof(*api_cfg));

    // Parse and fill api cfg
    fhash_str_iter iter = fhash_str_iter_new(node->data.mapping);
    sk_cfg_node_t* child = NULL;

    while ((child = fhash_str_next(&iter))) {
        const char* key = iter.key;
        bool mode_exist = false;

        if (0 == strcmp(key, "mode")) {
            mode_exist = true;
            const char* value = sk_config_getstring(child);

            if (0 == strcmp(value, "read")) {
                api_cfg->access_mode = SK_SRV_API_READ;
            } else if (0 == strcmp(value, "write")) {
                api_cfg->access_mode = SK_SRV_API_WRITE;
            } else {
                sk_print_err("Fatal: unknown service '%s' api '%s' access mode "
                             "'%s', use read or write\n",
                             service_name, api_name, value);
                exit(1);
            }
        } else {
            sk_print_err("Fatal: unknown service '%s' api '%s' cfg field "
                         "'%s'\n", service_name, api_name, key);
            exit(1);
        }

        if (!mode_exist) {
            sk_print_err("Fatal: service '%s' api '%s' cfg missing mode "
                         "field\n", service_name, api_name);
            exit(1);
        }
    }

    fhash_str_iter_release(&iter);

    // Add api cfg item to apis table
    fhash_str_set(cfg->apis, api_name, api_cfg);
}

static
void _load_service_apis(const sk_cfg_node_t* node, const char* service_name,
                        sk_service_cfg_t* cfg)
{
    SK_ASSERT_MSG(node->type == SK_CFG_NODE_MAPPING,
                  "service api item config must be a mapping\n");

    fhash_str_iter iter = fhash_str_iter_new(node->data.mapping);
    sk_cfg_node_t* child = NULL;

    while ((child = fhash_str_next(&iter))) {
        const char* api_name = iter.key;
        _load_service_api(child, service_name, api_name, cfg);
    }

    fhash_str_iter_release(&iter);
}

static
bool _validate_service_cfg(const char* service_name,
                           const sk_service_cfg_t* service_cfg)
{
    fhash_str_iter api_iter = fhash_str_iter_new(service_cfg->apis);
    const sk_srv_api_cfg_t* api_cfg = NULL;

    while ((api_cfg = fhash_str_next(&api_iter))) {
        //const char* api_name = api_iter.key;
        //sk_print("validating api config: %s\n", api_name);
    }
    fhash_str_iter_release(&api_iter);

    return true;
}

static
void _load_service(const char* service_name, sk_cfg_node_t* node,
                   sk_config_t* config)
{
    SK_ASSERT_MSG(node->type == SK_CFG_NODE_MAPPING,
                  "service item config must be a mapping\n");

    sk_service_cfg_t* service_cfg = _service_cfg_item_create();
    fhash_str_iter iter = fhash_str_iter_new(node->data.mapping);
    sk_cfg_node_t* child = NULL;
    bool data_mode_exist = false;

    while ((child = fhash_str_next(&iter))) {
        const char* key = iter.key;

        if (0 == strcmp(key, "enable")) {
            service_cfg->enable = sk_config_getint(child);
        } else if (0 == strcmp(key, "data_mode")) {
            data_mode_exist = true;
            _load_service_data_mode(child, service_cfg);
        } else if (0 == strcmp(key, "apis")) {
            _load_service_apis(child, service_name, service_cfg);
        }
    }

    fhash_str_iter_release(&iter);

    // check required fields
    if (!data_mode_exist) {
        sk_print_err("Fatal: service (%s) cfg missing data_mode field\n",
                     service_name);
        exit(1);
    }

    // validate the whole service config
    if (!_validate_service_cfg(service_name, service_cfg)) {
        sk_print_err("Fatal: service (%s) config is incorrect\n",
                     service_name);
        exit(1);
    }

    fhash_str_set(config->services, service_name, service_cfg);
}

static
void _load_services(sk_cfg_node_t* node, sk_config_t* config)
{
    if (node->type != SK_CFG_NODE_MAPPING) {
        sk_print_err("service config must be a mapping, skip it\n");
        return;
    }

    sk_cfg_node_t* child = NULL;
    fhash_str_iter iter = fhash_str_iter_new(node->data.mapping);

    while ((child = fhash_str_next(&iter))) {
        const char* service_name = iter.key;

        _load_service(service_name, child, config);
    }

    fhash_str_iter_release(&iter);
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

        // load services
        if (0 == strcmp(key, "services")) {
            _load_services(child, config);
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
