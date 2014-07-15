#include <stdlib.h>
#include <stdio.h>

#include "fev/fev_buff.h"
#include "api/sk_event.h"
#include "api/sk_sched.h"

// consume the data received from network
static
int _req(sk_sched_t* sched, sk_event_t* event)
{
    char* data = event->data.d.ud;
    size_t sz = event->data.sz;
    printf("receive data: [%s], sz=%zu\n", data, sz);

    return 0;
}

static
int _req_end(sk_sched_t* sched, sk_event_t* event)
{
    printf("req event end\n");
    return 1;
}

static
void _destroy(sk_sched_t* sched, sk_event_t* event)
{
    free(event->data.d.ud);
}

sk_event_opt_t sk_pto_net_proc = {
    .req  = _req,
    .resp = NULL,
    .req_end  = _req_end,
    .resp_end = NULL,
    .destroy  = _destroy
};
