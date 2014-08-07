#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "fev/fev_buff.h"
#include "api/sk_utils.h"
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
    char* tmp = calloc(1, sz + 1);
    memcpy(tmp, data, sz);
    sk_print("receive data: [%s], sz=%zu\n", tmp, sz);
    sk_print("echo back the data [%s]\n", tmp);
    sk_entity_write(entity, data, sz);
    free(tmp);

    return 0;
}

sk_proto_t sk_pto_net_proc = {
    .priority = SK_PTO_PRI_5,
    .descriptor = &net_proc__descriptor,
    .run = _req
};
