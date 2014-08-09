#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <yaml.h>

#include "api/sk_utils.h"
#include "api/sk_config_loader.h"

// INTERNAL APIs

static
sk_cfg_node_t* _sk_cfg_node_create(sk_cfg_node_type_t type, const char* value)
{
    sk_cfg_node_t* node = calloc(1, sizeof(*node));
    memset(node, 0, sizeof(*node));
    node->type = type;

    if (type == SK_CFG_NODE_VALUE) {
        node->data.value = strdup(value);
        node->size++;
    } else if (type == SK_CFG_NODE_ARRAY) {
        node->data.array = flist_create();
    } else if (type == SK_CFG_NODE_MAPPING) {
        node->data.mapping = fhash_str_create(0, FHASH_MASK_AUTO_REHASH);
    }

    return node;
}

static
void _sk_cfg_node_delete(sk_cfg_node_t* node)
{
    if (node->type == SK_CFG_NODE_VALUE) {
        free(node->data.value);
    } else if (node->type == SK_CFG_NODE_ARRAY) {
        flist_delete(node->data.array);
    } else {
        fhash_str_delete(node->data.mapping);
    }

    free(node);
}

static
void _sk_cfg_node_add(sk_cfg_node_t* parent,
                      const char* key, // only availible for mapping type
                      sk_cfg_node_t* child)
{
    SK_ASSERT(parent->type != SK_CFG_NODE_VALUE);

    if (parent->type == SK_CFG_NODE_ARRAY) {
        int ret = flist_push(parent->data.array, child);
        SK_ASSERT(!ret);
    } else {
        fhash_str_set(parent->data.mapping, key, child);
    }

    parent->size++;
}

static
void _print_prefix(int depth)
{
    for (int i = 0; i < depth; i++) {
        sk_rawprint("  ");
    }
}

static
sk_cfg_node_t* _node_process(int depth, yaml_document_t* doc,
                             yaml_node_t* parent)
{
    sk_cfg_node_t* cfg_parent = NULL;

    switch (parent->type) {
    case YAML_MAPPING_NODE:
    {
        _print_prefix(depth);
        sk_rawprint("[mapping] start\n");
        cfg_parent = _sk_cfg_node_create(SK_CFG_NODE_MAPPING, NULL);

        yaml_node_pair_t* pair = parent->data.mapping.pairs.start;
        for (; pair < parent->data.mapping.pairs.top; pair++) {
            // get the key node
            yaml_node_t* key_node = yaml_document_get_node(doc, pair->key);
            SK_ASSERT_MSG(key_node->type == YAML_SCALAR_NODE,
                          "key->type: \n", key_node->type);
            char* mapping_key = (char*)key_node->data.scalar.value;
            _print_prefix(depth);
            sk_rawprint("key: %s\n", mapping_key);

            // get the value node
            yaml_node_t* value_node = yaml_document_get_node(doc, pair->value);
            sk_cfg_node_t* child = _node_process(depth + 1, doc, value_node);
            _sk_cfg_node_add(cfg_parent, mapping_key, child);
        }

        _print_prefix(depth);
        sk_rawprint("[mapping] end\n");
        break;
    }
    case YAML_SEQUENCE_NODE:
    {
        _print_prefix(depth);
        sk_rawprint("[sequence] start\n");
        cfg_parent = _sk_cfg_node_create(SK_CFG_NODE_ARRAY, NULL);

        yaml_node_item_t* item = parent->data.sequence.items.start;
        for (; item < parent->data.sequence.items.top; item++) {
            yaml_node_t* sequence_node = yaml_document_get_node(doc, *item);

            sk_cfg_node_t* child = _node_process(depth + 1, doc, sequence_node);
            _sk_cfg_node_add(cfg_parent, NULL, child);
        }

        _print_prefix(depth);
        sk_rawprint("[sequence] end\n");
        break;
    }
    case YAML_SCALAR_NODE:
        _print_prefix(depth);
        sk_rawprint("node type: %d, value: %s\n",
                    parent->type, parent->data.scalar.value);

        cfg_parent = _sk_cfg_node_create(SK_CFG_NODE_VALUE,
                                         (char*)parent->data.scalar.value);
        break;
    default:
        _print_prefix(depth);
        sk_rawprint("un-processed type: %d\n", parent->type);
        break;
    }

    return cfg_parent;
}

static
sk_cfg_node_t* _document_process(yaml_parser_t* parser, const char* filename,
                                 int depth)
{
    yaml_document_t doc;
    yaml_node_t* root = NULL;
    int parse_done = 0;
    sk_cfg_node_t* cfg_root = NULL;

    do {
        int ret = yaml_parser_load(parser, &doc);
        SK_ASSERT_MSG(ret, "parse config %s failed: %s\n%s\n",
                      filename, parser->problem, parser->context);

        root = yaml_document_get_root_node(&doc);
        // get thread_num
        sk_print("root=%p\n", (void*)root);
        if (root) {
            sk_print("root->type: %d\n", root->type);

            cfg_root = _node_process(0, &doc, root);
        }

        parse_done = (root == NULL);
        yaml_document_delete(&doc);
    } while (!parse_done);

    return cfg_root;
}

static
void _sk_config_dump(int depth, sk_cfg_node_t* node)
{
    switch (node->type) {
    case SK_CFG_NODE_VALUE:
        _print_prefix(depth);
        sk_rawprint("value: %s\n", node->data.value);
        break;
    case SK_CFG_NODE_MAPPING:
    {
        fhash_str_iter iter = fhash_str_iter_new(node->data.mapping);
        sk_cfg_node_t* child = NULL;

        while ((child = fhash_str_next(&iter))) {
            _print_prefix(depth);
            sk_rawprint("key=%s\n", iter.key);
            _sk_config_dump(depth + 1, child);
        }

        fhash_str_iter_release(&iter);
        break;
    }
    case SK_CFG_NODE_ARRAY:
    {
        sk_cfg_node_t* child = NULL;
        flist_iter iter = flist_new_iter(node->data.array);
        while ((child = flist_each(&iter))) {
            _sk_config_dump(depth + 1, child);
        }
        break;
    }
    default:
        break;
    }
}

// APIs
sk_cfg_node_t* sk_config_load(const char* filename)
{
    // 1. prepare the yaml parser
    yaml_parser_t cfg_parser;

    int ret = yaml_parser_initialize(&cfg_parser);
    SK_ASSERT_MSG(ret, "yaml_parser_init failed, ret=%d\n", ret);

    FILE* cfg_file = fopen(filename, "r");
    SK_ASSERT_MSG(cfg_file, "open config %s failed: %s\n",
                  filename, strerror(errno));

    yaml_parser_set_input_file(&cfg_parser, cfg_file);

    // 2. process the yaml file
    //_token_process(&cfg_parser, 0, config);
    sk_cfg_node_t* root = _document_process(&cfg_parser, filename, 0);

    // 3. clean up the source of parser
    fclose(cfg_file);
    yaml_parser_delete(&cfg_parser);

    return root;
}

void sk_config_delete(sk_cfg_node_t* node)
{
    switch (node->type) {
    case SK_CFG_NODE_VALUE:
        _sk_cfg_node_delete(node);
        break;
    case SK_CFG_NODE_MAPPING:
    {
        fhash_str_iter iter = fhash_str_iter_new(node->data.mapping);
        sk_cfg_node_t* child = NULL;

        while ((child = fhash_str_next(&iter))) {
            sk_config_delete(child);
        }

        fhash_str_iter_release(&iter);
        _sk_cfg_node_delete(node);
        break;
    }
    case SK_CFG_NODE_ARRAY:
    {
        sk_cfg_node_t* child = NULL;
        flist_iter iter = flist_new_iter(node->data.array);
        while ((child = flist_each(&iter))) {
            sk_config_delete(child);
        }
        _sk_cfg_node_delete(node);
        break;
    }
    default:
        break;
    }
}

void sk_config_dump(sk_cfg_node_t* root)
{
    _sk_config_dump(0, root);
}
