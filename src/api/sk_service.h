#ifndef SK_SERVICE_H
#define SK_SERVICE_H

#include "api/sk_config.h"

typedef enum sk_service_type_t {
    SK_C_SERICE_TYPE = 0
} sk_service_type_t;

typedef struct sk_service_opt_t {
    void* srv_data;

    void (*init) (void* srv_data);
    void (*release) (void* srv_data);
} sk_service_opt_t;

typedef struct sk_service_t sk_service_t;

sk_service_t* sk_service_create(const char* service_name,
                                const sk_service_cfg_t* cfg);
void sk_service_destroy(sk_service_t*);

void sk_service_setopt(sk_service_t*, sk_service_opt_t opt);
void sk_service_settype(sk_service_t*, sk_service_type_t type);

void sk_service_start(sk_service_t*);
void sk_service_stop(sk_service_t*);

const char* sk_service_name(const sk_service_t*);
sk_service_type_t sk_service_type(const sk_service_t*);
const sk_service_cfg_t* sk_service_config(const sk_service_t*);

#endif

