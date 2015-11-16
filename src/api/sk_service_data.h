#ifndef SK_SERVICE_DATA_H
#define SK_SERVICE_DATA_H

typedef enum sk_srv_data_mode_t {
    SK_SRV_DATA_MODE_RW_PR = 0,     // prefer read
    SK_SRV_DATA_MODE_RW_PW = 1      // prefer write
} sk_srv_data_mode_t;

typedef struct sk_srv_data_t sk_srv_data_t;

sk_srv_data_t* sk_srv_data_create(sk_srv_data_mode_t mode);
void sk_srv_data_destroy(sk_srv_data_t*);

sk_srv_data_mode_t sk_srv_data_mode(sk_srv_data_t*);

void* sk_srv_data_get(sk_srv_data_t*);
const void* sk_srv_data_getconst(sk_srv_data_t*);
void sk_srv_data_set(sk_srv_data_t*, const void* data);

#endif

