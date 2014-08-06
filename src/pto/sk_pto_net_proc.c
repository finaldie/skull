#include <stdlib.h>
#include <stdio.h>

#include "fev/fev_buff.h"
#include "api/sk_event.h"
#include "api/sk_entity_mgr.h"
#include "api/sk_pto.h"
#include "api/sk_sched.h"

// consume the data received from network
static
int _req(sk_sched_t* sched, sk_entity_t* entity, void* proto_msg)
{
    NetProc* net_msg = proto_msg;
    size_t sz = net_msg->data.len;
    unsigned char* data = net_msg->data.data;
    printf("receive data: [%s], sz=%zu\n", data, sz);
    printf("echo back the data [%s]\n", data);
    sk_entity_write(entity, data, sz);

    return 0;
}

sk_proto_t sk_pto_net_proc = {
    .priority = 5,
    .descriptor = &net_proc__descriptor,
    .run = _req
};
