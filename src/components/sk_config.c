#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <errno.h>

#include "flibs/flog.h"

#include "api/sk_const.h"
#include "api/sk_utils.h"
#include "api/sk_env.h"
#include "api/sk_config_loader.h"
#include "api/sk_config.h"

static
sk_workflow_cfg_t* _create_workflow_cfg()
{
    sk_workflow_cfg_t* workflow = calloc(1, sizeof(*workflow));
    workflow->port    = SK_CONFIG_NO_PORT;
    workflow->modules = flist_create();
    workflow->bind    = strdup("0.0.0.0");
    return workflow;
}

static
void _delete_workflow_cfg(sk_workflow_cfg_t* workflow)
{
    sk_module_cfg_t* mcfg = NULL;
    while ((mcfg = flist_pop(workflow->modules))) {
        free((void*)mcfg->name);
        free((void*)mcfg->type);
        free(mcfg);
    }
    flist_delete(workflow->modules);

    free((void*)workflow->bind);
    free((void*)workflow->idl_name);
    free(workflow);
}

static
sk_service_cfg_t* _service_cfg_item_create()
{
    sk_service_cfg_t* cfg = calloc(1, sizeof(*cfg));
    cfg->enable = false;
    cfg->data_mode = SK_SRV_DATA_MODE_RW_PR;
    cfg->max_qsize = 0;

    return cfg;
}

static
void _service_cfg_item_destroy(sk_service_cfg_t* cfg)
{
    if (!cfg) {
        return;
    }

    free((void*)cfg->type);
    free(cfg);
}

static
sk_config_t* _create_config()
{
    sk_config_t* config = calloc(1, sizeof(*config));
    config->threads   = 1;
    config->log_level = FLOG_LEVEL_INFO;
    config->workflows = flist_create();
    config->services  = fhash_str_create(0, FHASH_MASK_AUTO_REHASH);
    config->command_port = SK_CONFIG_DEFAULT_CMD_PORT;
    config->langs     = flist_create();
    config->max_fds   = SK_DEFAULT_OPEN_FILES;
    strncpy(config->log_name, "skull.log", sizeof("skull.log"));
    strncpy(config->diag_name, "diag.log", sizeof("diag.log"));
    config->txn_logging = false;

    return config;
}

static
void _delete_config(sk_config_t* config)
{
    // destroy workflow
    sk_workflow_cfg_t* workflow = NULL;

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

    // destroy langs
    char* lang = NULL;
    while ((lang = flist_pop(config->langs))) {
        free(lang);
    }
    flist_delete(config->langs);

    free((void*)config->command_bind);
    free(config);
}

static
void _load_modules(sk_cfg_node_t* node, sk_workflow_cfg_t* workflow)
{
    if (node->type != SK_CFG_NODE_ARRAY) {
        sk_print("Not a valid module item, won't load it\n");
        return;
    }

    sk_cfg_node_t* child = NULL;
    flist_iter iter = flist_new_iter(node->data.array);
    while ((child = flist_each(&iter))) {
        SK_ASSERT(child->type == SK_CFG_NODE_VALUE);

        sk_module_cfg_t* mcfg = calloc(1, sizeof(*mcfg));
        const char* raw = child->data.value;

        char tmp[SK_CONFIG_VALUE_MAXLEN];
        strncpy(tmp, raw, SK_CONFIG_VALUE_MAXLEN);
        char* tmp1 = tmp;

        // Format: 'name:type'
        char* token = NULL;
        int i = 0;
        while ((token = strsep(&tmp1, ":")) && i < 2) {
            if (i == 0) {
                // fill name
                mcfg->name = strdup(token);
            } else {
                mcfg->type = strdup(token);
            }

            i++;
        }

        SK_ASSERT_MSG(i == 2, "Invalid module format, correct it. "
                      "Format: 'name:type', content: %s\n", raw);

        int ret = flist_push(workflow->modules, mcfg);
        SK_ASSERT(!ret);
    }
}

// example yaml config:
// workflow:
//     - modules: [test]
//       concurrency: 1
//       port: 7758
static
void _load_workflow(sk_cfg_node_t* node, sk_config_t* config)
{
    if (node->type != SK_CFG_NODE_ARRAY) {
        sk_print("Not a valid workflow item, won't load it\n");
        return;
    }

    int enabled_stdin = 0;
    sk_cfg_node_t* child = NULL;
    flist_iter iter = flist_new_iter(node->data.array);

    while ((child = flist_each(&iter))) {
        SK_ASSERT_MSG(child->type == SK_CFG_NODE_MAPPING,
                      "workflow sequence item must be a mapping\n");

        sk_workflow_cfg_t* workflow = _create_workflow_cfg();

        // load modules and concurrency
        fhash_str_iter item_iter = fhash_str_iter_new(child->data.mapping);
        while ((child = fhash_str_next(&item_iter))) {
            const char* key = item_iter.key;

            if (0 == strcmp(key, "modules")) {
                _load_modules(child, workflow);
            } else if (0 == strcmp(key, "idl")) {
                workflow->idl_name = strdup(child->data.value);
            } else if (0 == strcmp(key, "concurrency")
                       || 0 == strcmp(key, "concurrent")) {
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

                if (workflow->enable_stdin) {
                    enabled_stdin = 1;
                }
            } else if (0 == strcmp(key, "bind")) {
                free((void*)workflow->bind);
                workflow->bind = strdup(sk_config_getstring(child));
            } else if (0 == strcmp(key, "timeout")) {
                workflow->timeout = sk_config_getint(child);
            } else if (0 == strcmp(key, "sock_type")) {
                int sock_type = SK_SOCK_TCP;
                const char* cfg_sock_type = sk_config_getstring(child);

                if (0 == strcasecmp("tcp", cfg_sock_type)) {
                    sock_type = SK_SOCK_TCP;
                } else if (0 == strcasecmp("udp", cfg_sock_type)) {
                    sock_type = SK_SOCK_UDP;
                } else {
                    SK_ASSERT_MSG(0, "sock_type must be 'tcp' or 'udp'\n");
                }

                workflow->sock_type = (uint32_t)sock_type & 0x1F;
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
        strncpy(config->log_name, log_name, SK_CONFIG_LOGNAME_LEN);
    } else {
        sk_print_err("Fatal: empty log name, please configure a non-empty "
                     "log name\n");
        exit(1);
    }
}

static
void _load_diaglog_name(sk_cfg_node_t* child, sk_config_t* config)
{
    SK_ASSERT(child->type == SK_CFG_NODE_VALUE);
    const char* log_name = child->data.value;

    if (log_name && strlen(log_name)) {
        strncpy(config->diag_name, log_name, SK_CONFIG_LOGNAME_LEN);
    } else {
        sk_print_err("Fatal: empty diag log name, please configure a non-empty "
                     "diag log name\n");
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
bool _validate_service_cfg(const char* service_name,
                           const sk_service_cfg_t* service_cfg)
{
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
            service_cfg->enable = sk_config_getbool(child);
        } else if (0 == strcmp(key, "data_mode")) {
            data_mode_exist = true;
            _load_service_data_mode(child, service_cfg);
        } else if (0 == strcmp(key, "type")) {
            service_cfg->type = strdup(sk_config_getstring(child));
        } else if (0 == strcmp(key, "max_qsize")) {
            service_cfg->max_qsize = sk_config_getint(child);
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
void _load_bios(sk_cfg_node_t* node, sk_config_t* config)
{
    if (node->type != SK_CFG_NODE_VALUE) {
        sk_print("Not a valid bio item, won't load it\n");
        return;
    }

    config->bio_cnt = sk_config_getint(node);
    SK_ASSERT_MSG(config->bio_cnt >= 0, "config: bg_iothreads must >= 0\n");
}

static
void _load_langs(sk_cfg_node_t* node, sk_config_t* config)
{
    if (node->type != SK_CFG_NODE_ARRAY) {
        sk_print("Not a valid language item, won't load it\n");
        return;
    }

    sk_cfg_node_t* child = NULL;
    flist_iter iter = flist_new_iter(node->data.array);
    while ((child = flist_each(&iter))) {
        SK_ASSERT(child->type == SK_CFG_NODE_VALUE);

        int ret = flist_push(config->langs, strdup(child->data.value));
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
        const char* key = iter.key;

        // load thread_num
        if (0 == strcmp(key, "worker_threads")) {
            config->threads = sk_config_getint(child);
            SK_ASSERT_MSG(config->threads > 0, "config: worker_threads must > 0\n");
        } else if (0 == strcmp(key, "workflows")) {
            // load working flows
            _load_workflow(child, config);
        } else if (0 == strcmp(key, "log_name")) {
            // load log name
            _load_log_name(child, config);
        } else if (0 == strcmp(key, "diag_log")) {
            // load mem log name
            _load_diaglog_name(child, config);
        } else if (0 == strcmp(key, "log_level")) {
            // load log level: trace|debug|info|warn|error|fatal
            _load_log_level(child, config);
        } else if (0 == strcmp(key, "services")) {
            // load services
            _load_services(child, config);
        } else if (0 == strcmp(key, "bg_iothreads")) {
            // load bio(s)
            _load_bios(child, config);
        } else if (0 == strcmp(key, "languages")) {
            // load languages
            _load_langs(child, config);
        } else if (0 == strcmp(key, "command_port")) {
            // load command port
            int port = sk_config_getint(child);
            SK_ASSERT_MSG(port > 0 && port <= 65535, "port[%d] should be "
                              "in (0, 65535]\n", port);
            config->command_port = port;
        } else if (0 == strcmp(key, "command_bind")) {
            config->command_bind = strdup(sk_config_getstring(child));
        } else if (0 == strcmp(key, "max_fds")) {
            // load max_fds
            int max_fds = sk_config_getint(child);
            config->max_fds = max_fds;
        } else if (0 == strcmp(key, "txn_logging")) {
            config->txn_logging = sk_config_getbool(child);
        } else if (0 == strcmp(key, "slowlog_ms")) {
            config->slowlog_ms = sk_config_getint(child);
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

#ifdef SK_DEBUG
    sk_config_dump(root);
#endif

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
    sk_print("diag_name: %s\n", config->diag_name);
    sk_print("log_level: %d\n", config->log_level);

    sk_workflow_cfg_t* workflow = NULL;
    flist_iter iter = flist_new_iter(config->workflows);
    while ((workflow = flist_each(&iter))) {
        sk_print("workflow start:\n");
        sk_print("\tconcurrency: %d\n", workflow->concurrent);
        sk_print("\tmodules: ");

        sk_module_cfg_t* mcfg = NULL;
        flist_iter name_iter = flist_new_iter(workflow->modules);
        while ((mcfg = flist_each(&name_iter))) {
            sk_rawprint("%s:%s ", mcfg->name, mcfg->type);
        }

        sk_rawprint("\n");
        sk_print("workflow end:\n");
    }
}
