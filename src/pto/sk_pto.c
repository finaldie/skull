#include <stdlib.h>

#include "api/sk_event.h"

sk_event_opt_t* sk_event_tbl[] = {
    &sk_pto_net_accept,     // SK_PTO_NET_ACCEPT
    &sk_pto_net_proc,       // SK_PTO_NET_PROC
    NULL
};
