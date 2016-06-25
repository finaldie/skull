#include <stdlib.h>

#include "api/sk_utils.h"
#include "api/sk_service_data.h"

struct sk_srv_data_t {
    sk_srv_data_mode_t mode;

#if __WORDSIZE == 64
    int _padding;
#endif

    void* data; // User maintain it, user is responisble for destroy it
};

sk_srv_data_t* sk_srv_data_create(sk_srv_data_mode_t mode)
{
    sk_srv_data_t* srv_data = calloc(1, sizeof(*srv_data));
    srv_data->mode = mode;
    return srv_data;
}

void sk_srv_data_destroy(sk_srv_data_t* srv_data)
{
    if (!srv_data) {
        return;
    }

    free(srv_data);
}

sk_srv_data_mode_t sk_srv_data_mode(sk_srv_data_t* srv_data)
{
    SK_ASSERT(srv_data);
    return srv_data->mode;
}

void* sk_srv_data_get(sk_srv_data_t* srv_data)
{
    SK_ASSERT(srv_data);
    return srv_data->data;
}

const void* sk_srv_data_getconst(sk_srv_data_t* srv_data)
{
    SK_ASSERT(srv_data);
    return srv_data->data;
}

void sk_srv_data_set(sk_srv_data_t* srv_data, const void* data)
{
    SK_ASSERT(srv_data);
    srv_data->data = (void*) data;
}
