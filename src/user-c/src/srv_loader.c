#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

#include "api/sk_utils.h"
#include "api/sk_loader.h"
#include "api/sk_txn.h"
#include "srv_executor.h"

#include "skull/txn.h"
#include "srv_loader.h"

const char* _srv_name (const char* short_name, char* fullname, size_t sz)
{
    return NULL;
}

const char* _srv_conf_name (const char* short_name, char* confname, size_t sz)
{
    return NULL;
}

int _srv_open (const char* filename, sk_service_opt_t* opt/*out*/)
{
    return 0;
}

int _srv_close (sk_service_t* service)
{
    return 0;
}

int _srv_load_config (sk_service_t* service, const char* user_cfg_filename)
{
    return 0;
}

sk_service_loader_t sk_c_service_loader = {
    .type        = SK_C_SERVICE_TYPE,
    .name        = _srv_name,
    .conf_name   = _srv_conf_name,
    .load_config = _srv_load_config,
    .open        = _srv_open,
    .close       = _srv_close
};
