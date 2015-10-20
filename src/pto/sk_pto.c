#include <stdlib.h>

#include "api/sk_pto.h"

sk_proto_t* sk_pto_tbl[] = {
    &sk_pto_net_accept,         // SK_PTO_NET_ACCEPT
    &sk_pto_workflow_run,       // SK_PTO_WORKFLOW_RUN
    &sk_pto_net_destroy,        // SK_PTO_NET_DESTROY
    &sk_pto_srv_iocall,         // SK_PTO_SERVICE_TASK_RUN
    &sk_pto_srv_task_run,       // SK_PTO_SERVICE_TASK_RUN
    &sk_pto_srv_task_complete,  // SK_PTO_SERVICE_TASK_COMPLETE
    NULL
};
