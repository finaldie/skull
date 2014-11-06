#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "fhash/fhash.h"
#include "api/sk_utils.h"
#include "api/sk_config_loader.h"
#include "api/sk_log_tpl.h"

// =============================== log template ================================
// The error log template format:
//
// 1:
//  msg: No such file or directory
//  solution: Check the input paramter which must not contain '&' character
// 2:
//  ...

typedef struct _log_tpl_item_t {
    const char* msg;
    const char* solution;
} _log_tpl_item_t;

struct sk_log_tpl_t {
    fhash* templates; // key: errno, value: _log_tpl_item
    sk_log_tpl_type_t type;

#if __WORDSIZE == 64
    int padding;
#endif
};

static
int _get_int(const char* int_string)
{
    long int value = strtol(int_string, NULL, 10);
    SK_ASSERT_MSG(errno != EINVAL && errno != ERANGE,
                  "load config errno: %d, %s\n", errno, strerror(errno));

    return (int)value;
}

static
_log_tpl_item_t* _log_tpl_item_create()
{
    _log_tpl_item_t* item = calloc(1, sizeof(*item));
    return item;
}

static
void _log_tpl_item_destroy(_log_tpl_item_t* item)
{
    if (!item) {
        return;
    }

    free((void*)item->msg);
    free((void*)item->solution);
    free(item);
}

static
void _load_one_error_msg(int log_id, sk_cfg_node_t* node, sk_log_tpl_t* tpl)
{
    SK_ASSERT_MSG(node->type == SK_CFG_NODE_MAPPING,
                  "node type: %d\n", node->type);

    // get msg and solution field, then store in tpl hash table
    _log_tpl_item_t* item = _log_tpl_item_create();

    fhash_str_iter iter = fhash_str_iter_new(node->data.mapping);
    sk_cfg_node_t* child = NULL;

    while ((child = fhash_str_next(&iter))) {
        SK_ASSERT_MSG(child->type == SK_CFG_NODE_VALUE,
                      "child node type: %d\n", child->type);

        const char* key = iter.key;

        if (0 == strcmp(key, "msg")) {
            item->msg = strdup(child->data.value);
        } else if (0 == strcmp(key, "msg")) {
            item->solution = strdup(child->data.value);
        } else {
            sk_print("unknown field in log template %s\n", key);
        }
    }

    fhash_str_iter_release(&iter);
    fhash_int_set(tpl->templates, log_id, item);

    sk_print("load error log template, log id: %d\n", log_id);
    sk_print(" \\_ msg: %s\n", item->msg);
    sk_print(" \\_ solution: %s\n", item->solution);
}

static
void _load_error_log_tpls(const char* filename, sk_log_tpl_t* tpl)
{
    sk_cfg_node_t* root = sk_config_load(filename);
    sk_config_dump(root);

    // load error log template infos to sk_logger_tpl structure
    SK_ASSERT_MSG(root->type == SK_CFG_NODE_MAPPING,
                  "root node type: %d\n", root->type);

    fhash_str_iter iter = fhash_str_iter_new(root->data.mapping);
    sk_cfg_node_t* child = NULL;

    while ((child = fhash_str_next(&iter))) {
        SK_ASSERT_MSG(child->type == SK_CFG_NODE_MAPPING,
                      "child node type: %d\n", child->type);

        const char* key = iter.key;
        int log_id = _get_int(key);
        _load_one_error_msg(log_id, child, tpl);
    }

    fhash_str_iter_release(&iter);
    sk_config_delete(root);
}

static
void _load_one_info_msg(int log_id, sk_cfg_node_t* child, sk_log_tpl_t* tpl)
{
    _log_tpl_item_t* item = _log_tpl_item_create();
    item->msg = strdup(child->data.value);

    fhash_int_set(tpl->templates, log_id, item);
    sk_print("load info log template, log id: %d\n", log_id);
    sk_print(" \\_ msg: %s\n", item->msg);
}

static
void _load_info_log_tpls(const char* filename, sk_log_tpl_t* tpl)
{
    sk_cfg_node_t* root = sk_config_load(filename);
    sk_config_dump(root);

    // load error log template infos to sk_logger_tpl structure
    SK_ASSERT_MSG(root->type == SK_CFG_NODE_MAPPING,
                  "root node type: %d\n", root->type);

    fhash_str_iter iter = fhash_str_iter_new(root->data.mapping);
    sk_cfg_node_t* child = NULL;

    while ((child = fhash_str_next(&iter))) {
        SK_ASSERT_MSG(child->type == SK_CFG_NODE_VALUE,
                      "child node type: %d\n", child->type);

        const char* key = iter.key;
        int log_id = _get_int(key);
        _load_one_info_msg(log_id, child, tpl);
    }

    fhash_str_iter_release(&iter);
    sk_config_delete(root);
}

static
void _destory_log_tpls(sk_log_tpl_t* tpl)
{
    fhash_int_iter iter = fhash_int_iter_new(tpl->templates);
    _log_tpl_item_t* item = NULL;

    while ((item = fhash_int_next(&iter))) {
        _log_tpl_item_destroy(item);
    }

    fhash_int_iter_release(&iter);
    fhash_int_delete(tpl->templates);
    free(tpl);
}

sk_log_tpl_t* sk_log_tpl_create(const char* filename, sk_log_tpl_type_t type)
{
    sk_log_tpl_t* tpl = calloc(1, sizeof(*tpl));
    tpl->templates = fhash_int_create(0, FHASH_MASK_AUTO_REHASH);
    tpl->type = type;

    if (type == SK_LOG_INFO_TPL) {
        _load_info_log_tpls(filename, tpl);
    } else {
        _load_error_log_tpls(filename, tpl);
    }

    return tpl;
}

void sk_log_tpl_destroy(sk_log_tpl_t* tpl)
{
    if (!tpl) {
        return;
    }

    _destory_log_tpls(tpl);
}

const char* sk_log_tpl_msg(sk_log_tpl_t* tpl, int log_id)
{
    _log_tpl_item_t* item = fhash_int_get(tpl->templates, log_id);
    if (!item) {
        return "";
    }

    return item->msg;
}

const char* sk_log_tpl_solution(sk_log_tpl_t* tpl, int log_id)
{
    _log_tpl_item_t* item = fhash_int_get(tpl->templates, log_id);
    if (!item) {
        return "";
    }

    return item->solution;
}
