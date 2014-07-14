#include <stdlib.h>

#include "api/sk_event.h"
#include "api/sk_pto.h"

sk_event_opt_t* sk_event_tbl[] = {
    &sk_pto_net_accept,
    NULL
};
