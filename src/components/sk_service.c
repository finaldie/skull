#include <stdlib.h>

#include "api/sk_service.h"

struct sk_service_t {
    sk_service_type_t type;

#if __WORDSIZE == 64
    int padding;
#endif

    const char*       name; // a ref
    sk_service_opt_t  opt;
    const sk_service_cfg_t* cfg;

    void* srv_data;
};

sk_service_t* sk_service_create(const char* service_name,
                                const sk_service_cfg_t* cfg)
{
    if (!service_name) {
        return NULL;
    }

    sk_service_t* service = calloc(1, sizeof(*service));
    service->name = service_name;

    return service;
}

void sk_service_destroy(sk_service_t* service)
{
    if (!service) {
        return;
    }

    free(service);
}

void sk_service_setopt(sk_service_t* service, sk_service_opt_t opt)
{
    service->opt = opt;
}

void sk_service_settype(sk_service_t* service, sk_service_type_t type)
{
    service->type = type;
}

void sk_service_start(sk_service_t* service)
{
    service->opt.init(service->opt.srv_data);
}

void sk_service_stop(sk_service_t* service)
{
    service->opt.release(service->opt.srv_data);
}

const char* sk_service_name(const sk_service_t* service)
{
    return service->name;
}

sk_service_type_t sk_service_type(const sk_service_t* service)
{
    return service->type;
}

const sk_service_cfg_t* sk_service_config(const sk_service_t* service)
{
    return service->cfg;
}
