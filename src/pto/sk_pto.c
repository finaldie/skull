#include <stdlib.h>

#include "api/sk_pto.h"

sk_proto_t* sk_pto_tbl[] = {
    &sk_pto_net_accept,         // SK_PTO_NET_ACCEPT
    &sk_pto_workflow_run,       // SK_PTO_WORKFLOW_RUN
    &sk_pto_entity_destroy,     // SK_PTO_NET_DESTROY
    &sk_pto_srv_iocall,         // SK_PTO_SERVICE_TASK_RUN
    &sk_pto_srv_task_run,       // SK_PTO_SERVICE_TASK_RUN
    &sk_pto_srv_task_complete,  // SK_PTO_SERVICE_TASK_COMPLETE
    &sk_pto_timer_triggered,    // SK_PTO_TIMER_TRIGGERED
    &sk_pto_timer_emit,         // SK_PTO_TIMER_EMIT
    &sk_pto_stdin_start,        // SK_PTO_STDIN_START
    &sk_pto_svc_timer_run,      // SK_PTO_SVC_TIMER_RUN
    &sk_pto_svc_timer_complete, // SK_PTO_SVC_TIMER_COMPLETE
    &sk_pto_srv_task_cb,        // SK_PTO_SVC_TASK_CB
    NULL
};
