#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/eventfd.h>
#include <google/protobuf-c/protobuf-c.h>

#include "fmbuf/fmbuf.h"
#include "fev/fev.h"
#include "api/sk_utils.h"
#include "api/sk_pto.h"
#include "api/sk_event.h"
#include "api/sk_eventloop.h"
#include "api/sk_io.h"
#include "api/sk_entity_mgr.h"
#include "api/sk_sched.h"

#define SK_SCHED_PULL_NUM   65536

#define SK_SCHED_MAX_IO_BRIDGE 48

// SK_IO_BRIDGE
typedef struct sk_io_bridge_t {
    sk_sched_t* dst;
    fmbuf*   mq;
    int      evfd;

#if __WORDSIZE == 64
    int        padding;
#endif
} sk_io_bridge_t;

// SCHEDULER
struct sk_sched_t {
    void* evlp;
    sk_entity_mgr_t* entity_mgr;
    sk_proto_t**     pto_tbl;
    sk_io_t*         io_tbl[SK_PTO_PRI_SZ];
    sk_io_bridge_t*  bridge_tbl[SK_SCHED_MAX_IO_BRIDGE];

    uint32_t   bridge_size;
    uint32_t   running:1;
    uint32_t   _reserved:31;
};

// INTERNAL APIs

// copy the events from bridge mq
// NOTES: this copy action will be executed on the dst eventloop thread,
//        here we only can do fetch and copy, DO NOT PUSH events into it
static
void _copy_event(fev_state* fev, int fd, int mask, void* arg)
{
    sk_io_bridge_t* io_bridge = arg;
    uint64_t event_cnt = 0;

    if (eventfd_read(fd, &event_cnt)) {
        // some error occured
        return;
    }

    if (!event_cnt) {
        return;
    }

    sk_print("_copy_event: %d\n", (int)event_cnt);
    sk_event_t event;

    // push the event to the corresponding sk_io
    while (!fmbuf_pop(io_bridge->mq, &event, SK_EVENT_SZ)) {
        sk_sched_t* dst = io_bridge->dst;
        int pto_id = event.pto_id;
        sk_proto_t* pto = dst->pto_tbl[pto_id];
        int priority = pto->priority;
        sk_io_t* dst_io = io_bridge->dst->io_tbl[priority];
        sk_print("dst_io: %p\n", (void*)dst_io);

        sk_io_push(dst_io, SK_IO_INPUT, &event, 1);
    }
}

static
void _setup_bridge(sk_sched_t* dst, sk_io_bridge_t* io_bridge)
{
    int ret = fev_reg_event(dst->evlp, io_bridge->evfd, FEV_READ,
                            _copy_event, NULL, io_bridge);
    SK_ASSERT(!ret);
}

static
sk_io_bridge_t* sk_io_bridge_create(sk_sched_t* dst, int size)
{
    sk_io_bridge_t* io_bridge = malloc(sizeof(*io_bridge));
    io_bridge->dst = dst;
    io_bridge->mq = fmbuf_create(SK_EVENT_SZ * size);
    io_bridge->evfd = eventfd(0, EFD_NONBLOCK);

    _setup_bridge(dst, io_bridge);

    return io_bridge;
}

static
void sk_io_bridge_destroy(sk_io_bridge_t* io_bridge)
{
    close(io_bridge->evfd);
    free(io_bridge);
}

// working flow:
// 1. try to copy *ONE* the events from src_io into io_bridge's mq
// 2. notify the dst eventloop how many events from io_bridge's mq
// 3. dst eventloop receive the notification, then fetch the events to dst io's
//    input mq
// 4. waiting for dst's scheduler to consume the events
//
// return how many events be delivered
static
int sk_io_bridge_deliver(sk_io_bridge_t* io_bridge, sk_io_t* src_io)
{
    // calculate how many events can be transferred
    int event_cnt = sk_io_used(src_io, SK_IO_OUTPUT);
    if (event_cnt == 0) {
        return 0;
    }
    int free_slots = fmbuf_free(io_bridge->mq) / SK_EVENT_SZ;
    if (free_slots == 0) {
        return 0;
    }

    // copy events from src_io to io bridge's mq
    sk_event_t event;
    int nevents = sk_io_pull(src_io, SK_IO_OUTPUT, &event, 1);
    SK_ASSERT(nevents == 1);
    int ret = fmbuf_push(io_bridge->mq, &event, SK_EVENT_SZ);
    SK_ASSERT(!ret);

    return 1;
}

static
void _check_ptos(sk_proto_t** pto_tbl)
{
    if (!pto_tbl) {
        return;
    }

    for (int i = 0; pto_tbl[i] != NULL; i++) {
        sk_proto_t* pto = pto_tbl[i];
        SK_ASSERT(pto->run);
    }
}

static inline
sk_io_t* _get_io(sk_sched_t* sched, int priority)
{
    return sched->io_tbl[priority];
}

static
int _run_event(sk_sched_t* sched, sk_io_t* io, sk_event_t* event)
{
    uint32_t pto_id = event->pto_id;
    sk_proto_t* pto = sched->pto_tbl[pto_id];
    sk_print("pto_id = %u\n", pto_id);
    SK_ASSERT(pto);

    // create a base entity if there is no entity
    sk_entity_t* entity = sk_entity_mgr_get(sched->entity_mgr, event->fd);
    if (!entity) {
        sk_print("create a new entity(%d)\n", event->fd);
        entity = sk_entity_create(event->fd, sched);
        sk_entity_mgr_add(sched->entity_mgr, event->fd, entity);
    } else if (sk_entity_status(entity) != SK_ENTITY_ACTIVE) {
        sk_print("entity(%d) already dead\n", event->fd);
        return 0;
    }

    ProtobufCMessage* msg = protobuf_c_message_unpack(pto->descriptor,
                                                      NULL,
                                                      event->sz,
                                                      event->data);
    // TODO: should check the return value, and do something
    pto->run(sched, entity, msg);

    protobuf_c_message_free_unpacked(msg, NULL);
    free(event->data);

    // delete the entity if its status ~= ACTIVE
    if (sk_entity_status(entity) != SK_ENTITY_ACTIVE) {
        sk_entity_mgr_del(sched->entity_mgr, event->fd);
        sk_print("entity(%d) status=%d, will be deleted\n",
               event->fd,
               sk_entity_status(entity));
    }

    return 0;
}

static
void _pull_and_run(sk_sched_t* sched, sk_io_t* sk_io)
{
    sk_event_t events[SK_SCHED_PULL_NUM];
    int nevents = sk_io_pull(sk_io, SK_IO_INPUT, events, SK_SCHED_PULL_NUM);

    for (int i = 0; i < nevents; i++) {
        sk_event_t* event = &events[i];
        _run_event(sched, sk_io, event);
    }
}

static
int _emit_event(sk_sched_t* sched, int io_type, int fd,
                  int pto_id, void* proto_msg)
{
    sk_proto_t* pto = sched->pto_tbl[pto_id];

    sk_event_t event;
    event.fd = fd;
    event.pto_id = pto_id;
    event.sz = protobuf_c_message_get_packed_size(proto_msg);
    event.data = malloc(event.sz);
    size_t packed_sz = protobuf_c_message_pack(proto_msg, event.data);
    SK_ASSERT(packed_sz == (size_t)event.sz);

    sk_io_t* io = _get_io(sched, pto->priority);
    SK_ASSERT(io);

    sk_print("src_io: %p\n", (void*)io);
    sk_io_push(io, io_type, &event, 1);
    SK_ASSERT(sk_io_used(io, io_type) > 0);
    return 0;
}

static inline
void _io_bridget_notify(sk_io_bridge_t* io_bridge, uint64_t cnt)
{
    // notify dst eventloop the data is ready
    eventfd_write(io_bridge->evfd, cnt);
}

void _deliver_msg(sk_sched_t* sched)
{
    sk_print("delivery_msg: start\n");
    int delivery[SK_SCHED_MAX_IO_BRIDGE] = {0};

    for (int i = SK_PTO_PRI_MAX; i >= SK_PTO_PRI_MIN; i--) {
        sk_io_t* io = sched->io_tbl[i];

        for (uint32_t bri_idx = 0; bri_idx < sched->bridge_size; bri_idx++) {
            sk_io_bridge_t* io_bridge = sched->bridge_tbl[bri_idx];
            delivery[bri_idx] += sk_io_bridge_deliver(io_bridge, io);
        }
    }

    // notify dst eventloop the data is ready
    for (uint32_t bri_idx = 0; bri_idx < sched->bridge_size; bri_idx++) {
        sk_print("bri_idx: %u, cnt: %d\n", bri_idx, delivery[bri_idx]);
        sk_io_bridge_t* io_bridge = sched->bridge_tbl[bri_idx];
        _io_bridget_notify(io_bridge, delivery[bri_idx]);
    }
    sk_print("delivery_msg: end\n");
}

// APIs
sk_sched_t* sk_sched_create()
{
    sk_sched_t* sched = malloc(sizeof(*sched));
    memset(sched, 0, sizeof(*sched));
    sched->evlp = sk_eventloop_create();
    sched->entity_mgr = sk_entity_mgr_create(sched, 65535);
    sched->pto_tbl = sk_pto_tbl;
    sched->running = 0;

    // create the default io
    for (int i = SK_PTO_PRI_MIN; i <= SK_PTO_PRI_MAX; i++) {
        sched->io_tbl[i] = sk_io_create(0, 0);
    }

    _check_ptos(sched->pto_tbl);
    return sched;
}

void sk_sched_destroy(sk_sched_t* sched)
{
    for (int i = 0; i < SK_PTO_PRI_SZ; i++) {
        // destroy io
        sk_io_destroy(sched->io_tbl[i]);

        // destroy io bridge
        for (uint32_t bri_idx = 0; bri_idx < sched->bridge_size; bri_idx++) {
            sk_io_bridge_destroy(sched->bridge_tbl[bri_idx]);
        }
    }

    sk_entity_mgr_destroy(sched->entity_mgr);
    sk_eventloop_destroy(sched->evlp);
    free(sched);
}

void sk_sched_start(sk_sched_t* sched)
{
    sched->running = 1;

    do {
        // pull all io events and convert them to sched events
        int nprocessed = sk_eventloop_dispatch(sched->evlp, 1000);
        if (nprocessed <= 0 ) {
            continue;
        }

        // execuate all io bridge
        // NOTE: deliver the msg from highest priority mq to lowest priority mq
        _deliver_msg(sched);

        // execuate all io events
        // NOTE: run the events from highest priority mq to lowest priority mq
        for (int i = SK_PTO_PRI_MAX; i >= SK_PTO_PRI_MIN; i--) {
            _pull_and_run(sched, sched->io_tbl[i]);
        }

        // execuate all io bridge again
        _deliver_msg(sched);

        // clean dead entities
        sk_entity_mgr_clean_dead(sched->entity_mgr);
    } while (sched->running);
}

void sk_sched_stop(sk_sched_t* sched)
{
    sched->running = 0;
}

int sk_sched_setup_bridge(sk_sched_t* src, sk_sched_t* dst)
{
    // register a sk_io which only used for delivery message in both side
    if (src->bridge_size == SK_SCHED_MAX_IO_BRIDGE) {
        return 1;
    }

    sk_io_bridge_t* io_bridge = sk_io_bridge_create(dst, 65535);
    src->bridge_tbl[src->bridge_size++] = io_bridge;
    return 0;
}

// push a event into scheduler, and the scheduler will route it to the specific
// sk_io
int sk_sched_push(sk_sched_t* sched, int fd, int pto_id, void* proto_msg)
{
    return _emit_event(sched, SK_IO_INPUT, fd, pto_id, proto_msg);
}

int sk_sched_send(sk_sched_t* sched, int fd, int pto_id, void* proto_msg)
{
    return _emit_event(sched, SK_IO_OUTPUT, fd, pto_id, proto_msg);
}

void* sk_sched_eventloop(sk_sched_t* sched)
{
    return sched->evlp;
}
