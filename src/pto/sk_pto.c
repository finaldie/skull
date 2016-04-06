#include <stdlib.h>

#include "api/sk_pto.h"

sk_proto_t sk_pto_tbl[] = {
    {SK_PTO_NET_ACCEPT,            SK_PTO_PRI_7, &sk_pto_net_accept},
    {SK_PTO_WORKFLOW_RUN,          SK_PTO_PRI_5, &sk_pto_workflow_run},
    {SK_PTO_ENTITY_DESTROY,        SK_PTO_PRI_9, &sk_pto_entity_destroy},
    {SK_PTO_SVC_IOCALL,            SK_PTO_PRI_6, &sk_pto_srv_iocall},
    {SK_PTO_SVC_TASK_RUN,          SK_PTO_PRI_4, &sk_pto_srv_task_run},
    {SK_PTO_SVC_TASK_CB,           SK_PTO_PRI_6, &sk_pto_srv_task_cb},
    {SK_PTO_SVC_TASK_COMPLETE,     SK_PTO_PRI_6, &sk_pto_srv_task_complete},
    {SK_PTO_TIMER_TRIGGERED,       SK_PTO_PRI_8, &sk_pto_timer_triggered},
    {SK_PTO_TIMER_EMIT,            SK_PTO_PRI_8, &sk_pto_timer_emit},
    {SK_PTO_STDIN_START,           SK_PTO_PRI_7, &sk_pto_stdin_start},
    {SK_PTO_SVC_TIMER_RUN,         SK_PTO_PRI_8, &sk_pto_svc_timer_run},
    {SK_PTO_SVC_TIMER_COMPLETE,    SK_PTO_PRI_8, &sk_pto_svc_timer_complete},
    {-1, -1, NULL}
};
