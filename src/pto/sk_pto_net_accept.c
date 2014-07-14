#include <stdlib.h>

#include "fev/fev.h"
#include "api/sk_pto.h"

static
int _req(void* evlp, struct sk_sched_t* sched, sk_ud_t data)
{

    return 0;
}

static
int _resp(void* evlp, struct sk_sched_t* sched, sk_ud_t data)
{
    return 0;
}

static
int _end(void* evlp, struct sk_sched_t* sched, sk_ud_t data)
{
    return 0;
}

static
void _destroy(void* evlp, struct sk_sched_t* sched, sk_ud_t data)
{

}

sk_event_opt_t sk_pto_net_accept = {
    .req = _req,
    .resp = _resp,
    .end = _end,
    .destroy = _destroy
};
