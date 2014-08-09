#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "api/sk_utils.h"
#include "api/sk_config_loader.h"
#include "api/sk_config.h"

#define SK_CONFIG_PROCESS_KEY   0
#define SK_CONFIG_PROCESS_VALUE 1

void sk_load_config(sk_config_t* config)
{
    // 1. prepare the yaml parser
    const char* location = config->location;

    sk_cfg_node_t* root = sk_config_load(location);
    sk_config_dump(root);
    sk_config_delete(root);
}
