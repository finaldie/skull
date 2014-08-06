#include <stdlib.h>

#include "api/sk_pto.h"

sk_proto_t* sk_pto_tbl[] = {
    &sk_pto_net_accept,     // SK_PTO_NET_ACCEPT
    &sk_pto_net_proc,       // SK_PTO_NET_PROC
    NULL
};
