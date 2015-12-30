#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/eventfd.h>
#include <google/protobuf-c/protobuf-c.h>

#include "flibs/fmbuf.h"
#include "flibs/fev.h"

#include "api/sk_const.h"
#include "api/sk_utils.h"
#include "api/sk_pto.h"
#include "api/sk_event.h"
#include "api/sk_eventloop.h"
#include "api/sk_io.h"
#include "api/sk_entity_mgr.h"
#include "api/sk_env.h"
#include "api/sk_txn.h"
#include "api/sk_sched.h"

// SK_IO_BRIDGE
typedef struct sk_io_bridge_t {
    sk_sched_t* dst;
    fmbuf*   mq;
    int      evfd;

#if __WORDSIZE == 64
    int      _padding;
#endif
} sk_io_bridge_t;

// SCHEDULER
struct sk_sched_t {
    void* evlp;
    sk_entity_mgr_t* entity_mgr;
    sk_proto_t**     pto_tbl;
    sk_io_t*         io_tbl[SK_PTO_PRI_SZ];
    sk_io_bridge_t*  bridge_tbl[SK_SCHED_MAX_IO_BRIDGE];
    flist*           workflows;

    uint32_t   bridge_size;
    uint32_t   last_delivery_idx;
    uint32_t   running:1;
    uint32_t   _reserved:31;
    int        flags;

    // sk_event read buffer
    sk_event_t events_buffer[1];
};

// INTERNAL APIs
static
void _event_destroy(sk_event_t* event)
{
    if (!event) return;

    sk_print("destroy an event: {pto_id: %d, hop: %d}\n",
             event->pto_id, event->hop);
    free(event->data);
}

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

    // push/send the event to the corresponding sk_io
    while (!fmbuf_pop(io_bridge->mq, &event, SK_EVENT_SZ)) {
        sk_sched_t* dst      = io_bridge->dst;
        uint32_t    pto_id   = event.pto_id;
        sk_proto_t* pto      = dst->pto_tbl[pto_id];
        int         priority = pto->priority;
        sk_io_t*    dst_io   = io_bridge->dst->io_tbl[priority];

        sk_io_type_t io_type = event.dst == dst ? SK_IO_INPUT : SK_IO_OUTPUT;
        sk_io_push(dst_io, io_type, &event, 1);
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
sk_io_bridge_t* sk_io_bridge_create(sk_sched_t* dst, size_t size)
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
    if (!io_bridge) {
        return;
    }

    // clean up the events data in io bridge
    sk_event_t event;
    while (!fmbuf_pop(io_bridge->mq, &event, SK_EVENT_SZ)) {
        _event_destroy(&event);
    }

    fmbuf_delete(io_bridge->mq);
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
int sk_io_bridge_deliver(sk_io_bridge_t* io_bridge, sk_event_t* event)
{
    SK_ASSERT(io_bridge);
    SK_ASSERT(event);

    // copy events from src_io to io bridge's mq
    int ret = fmbuf_push(io_bridge->mq, event, SK_EVENT_SZ);
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

static inline
void _io_bridget_notify(sk_io_bridge_t* io_bridge, uint64_t cnt)
{
    if (cnt == 0) {
        return;
    }

    sk_print("delivery cnt: %d\n", (int) cnt);
    // notify dst eventloop the data is ready
    eventfd_write(io_bridge->evfd, cnt);
}

static
sk_io_bridge_t* _find_affinity_bridge(sk_sched_t* sched, sk_sched_t* target,
                                      uint32_t* index)
{
    SK_ASSERT(sched);
    SK_ASSERT(target);

    for (uint32_t bri_idx = 0; bri_idx < sched->bridge_size; bri_idx++) {
        sk_io_bridge_t* io_bridge = sched->bridge_tbl[bri_idx];

        if (io_bridge->dst != target) {
            continue;
        }

        if (index) {
            *index = bri_idx;
        }

        return io_bridge;
    }

    // doesn't find any affinity io bridge, possibly, this is a message sent
    // from worker to master
    return NULL;
}

// Find next routable io bridge by roundrobin alg
static
sk_io_bridge_t* _find_bridge_roundrobin(sk_sched_t* sched, uint32_t* bridge_idx)
{
    uint32_t end = sched->last_delivery_idx;
    int find = 0;

    do {
        uint32_t        idx       = sched->last_delivery_idx;
        sk_io_bridge_t* bridge    = sched->bridge_tbl[idx];
        int             dst_flags = bridge->dst->flags;

        if ((dst_flags & SK_SCHED_NON_RR_ROUTABLE) == 0) {
            find = 1;
            break;
        }

        // update the index
        sched->last_delivery_idx = (idx + 1) % sched->bridge_size;
    } while (sched->last_delivery_idx != end);

    if (!find) {
        return NULL;
    }

    *bridge_idx = sched->last_delivery_idx;
    SK_ASSERT(*bridge_idx < sched->bridge_size);

    sk_io_bridge_t* io_bridge = sched->bridge_tbl[*bridge_idx];
    sched->last_delivery_idx = (*bridge_idx + 1) % sched->bridge_size;

    sk_print("deliver to iobridge-%u, last_delivery_idx: %u, bridge_size: %u\n",
        *bridge_idx, sched->last_delivery_idx, sched->bridge_size);

    return io_bridge;
}

// Route a message to destination
static
void _deliver_one_io(sk_sched_t* sched, sk_io_t* src_io,
                     uint64_t* delivery)
{
    size_t nevents = 0;

    do {
        // 1. check & pull one event
        nevents = sk_io_used(src_io, SK_IO_OUTPUT);
        if (nevents == 0) {
            break;
        }

        sk_event_t event;
        nevents = sk_io_pull(src_io, SK_IO_OUTPUT, &event, 1);
        SK_ASSERT(nevents == 1);
        sk_print("prepare to deliver event, pto id: %lu\n", nevents);

        // 2. Drop event if it reach the max routing hop
        if (event.hop == SK_SCHED_MAX_ROUTING_HOP) {
            sk_print("stop routing this event due to it reach the max hop: %d\n",
                     SK_SCHED_MAX_ROUTING_HOP);
            _event_destroy(&event);
            continue;
        }

        // 3. get the affinity or round-robin io bridge
        sk_io_bridge_t* io_bridge = NULL;
        sk_sched_t* dst = event.dst;
        uint32_t bridge_index = 0;

        if (dst) {
            io_bridge = _find_affinity_bridge(sched, dst, &bridge_index);

            if (!io_bridge) {
                io_bridge = _find_bridge_roundrobin(sched, &bridge_index);
            }
        } else {
            io_bridge = _find_bridge_roundrobin(sched, &bridge_index);
            event.dst = io_bridge->dst;
        }

        if (!io_bridge) {
            sk_print("cannot find a io bridge to deliver\n");
            SK_LOG_WARN(SK_ENV_CORE->logger,
                "cannot find a io bridge to deliver, ptoid: %d", event.pto_id);

            _event_destroy(&event);
            continue;
        }

        // 4. Check capacity of dst
        size_t free_slots = fmbuf_free(io_bridge->mq) / SK_EVENT_SZ;
        if (free_slots == 0) {
            sk_print("no slot in the io bridge, drop it\n");
            _event_destroy(&event);
            continue;
        }

        // 5. deliver event
        if (io_bridge) {
            delivery[bridge_index] +=
                (uint64_t) sk_io_bridge_deliver(io_bridge, &event);
        }

        // 6. update hop
        event.hop++;
    } while (nevents > 0);
}

void _deliver_msg(sk_sched_t* sched)
{
    uint64_t delivery[sched->bridge_size];
    memset(delivery, 0, sizeof(delivery));

    // 1. deliver the events from higest priority to the lowest
    for (int i = SK_PTO_PRI_MAX; i >= SK_PTO_PRI_MIN; i--) {
        sk_io_t* io = sched->io_tbl[i];
        _deliver_one_io(sched, io, delivery);
    }

    // 2. notify dst eventloop the data is ready
    for (uint32_t bri_idx = 0; bri_idx < sched->bridge_size; bri_idx++) {
        sk_io_bridge_t* io_bridge = sched->bridge_tbl[bri_idx];
        _io_bridget_notify(io_bridge, delivery[bri_idx]);
    }
}

static
int _run_event(sk_sched_t* sched, sk_io_t* io, sk_event_t* event)
{
    SK_ASSERT(sched == event->dst);

    uint32_t pto_id = event->pto_id;
    sk_proto_t* pto = sched->pto_tbl[pto_id];
    sk_print("pto_id = %u\n", pto_id);
    SK_ASSERT(pto);

    sk_entity_t* entity = event->entity;
    SK_ASSERT(entity);

    // 1. Check the entity is active
    sk_entity_status_t status = sk_entity_status(entity);
    if (status != SK_ENTITY_ACTIVE) {
        sk_print("entity has already inactive or dead\n");
        return 0;
    }

    // 2. Add entity into entity_mgr
    if (NULL == sk_entity_owner(entity)) {
        sk_entity_mgr_add(sched->entity_mgr, entity);
    }

    // 3. Decode the message
    ProtobufCMessage* msg = protobuf_c_message_unpack(pto->descriptor,
                                                      NULL,
                                                      event->sz,
                                                      event->data);

    // 4. Run event
    sk_txn_t*   txn = event->txn;
    sk_sched_t* src = event->src;
    pto->run(sched, src, entity, txn, msg);

    // 5. Release message resources
    protobuf_c_message_free_unpacked(msg, NULL);
    _event_destroy(event);

    // 6. Delete the entity if its status ~= ACTIVE
    if (sk_entity_status(entity) != SK_ENTITY_ACTIVE) {
        sk_entity_mgr_del(sched->entity_mgr, entity);
        sk_print("entity status=%d, will be deleted\n",
                 sk_entity_status(entity));
    }

    return 0;
}

static
int _pull_and_run(sk_sched_t* sched, sk_io_t* sk_io)
{
    int nevents = (int)sk_io_pull(sk_io, SK_IO_INPUT, sched->events_buffer,
                             SK_SCHED_PULL_NUM);

    for (int i = 0; i < nevents; i++) {
        sk_event_t* event = &sched->events_buffer[i];
        _run_event(sched, sk_io, event);
    }

    return nevents;
}

static
int _process_events_once(sk_sched_t* sched)
{
    // execute all io bridges
    // NOTE: deliver the msg from highest priority mq to lowest priority mq
    _deliver_msg(sched);

    // execuate all io events
    // NOTE: run the events from highest priority mq to lowest priority mq
    int nprocessed = 0;
    for (int i = SK_PTO_PRI_MAX; i >= SK_PTO_PRI_MIN; i--) {
        nprocessed += _pull_and_run(sched, sched->io_tbl[i]);
    }

    // clean dead entities
    sk_entity_mgr_clean_dead(sched->entity_mgr);
    return nprocessed;
}

// process the events from all the priority queues until there is no active
// event left in all the queues
static
int _process_events(sk_sched_t* sched)
{
    int nprocessed = 0;

    do {
        nprocessed = _process_events_once(sched);
    } while (nprocessed > 0);

    return nprocessed;
}

static
int _emit_event(sk_sched_t* sched, sk_sched_t* dst, sk_io_type_t io_type,
                sk_entity_t* entity, sk_txn_t* txn,
                uint32_t pto_id, void* proto_msg, int flags)
{
    sk_proto_t* pto = sched->pto_tbl[pto_id];

    sk_event_t event;
    memset(&event, 0, sizeof(event));
    event.src    = sched;
    event.dst    = dst;
    event.pto_id = pto_id;
    event.hop    = 0;
    event.flags  = (uint32_t) flags & 0x0FFFFFFF;
    event.entity = entity;
    event.txn    = txn;

    if (proto_msg) {
        event.sz = protobuf_c_message_get_packed_size(proto_msg);
        event.data = malloc(event.sz);
        size_t packed_sz = protobuf_c_message_pack(proto_msg, event.data);
        SK_ASSERT(packed_sz == (size_t)event.sz);
    } else {
        event.sz = 0;
        event.data = NULL;
    }

    sk_io_t* io = _get_io(sched, pto->priority);
    SK_ASSERT(io);

    sk_io_push(io, io_type, &event, 1);
    SK_ASSERT(sk_io_used(io, io_type) > 0);
    return 0;
}

static
void _cleanup_skio_events(sk_io_t* io)
{
    sk_event_t event;
    while (sk_io_pull(io, SK_IO_INPUT, &event, 1)) {
        sk_print("clean up skio event from input queue\n");
        _event_destroy(&event);
    }

    while (sk_io_pull(io, SK_IO_OUTPUT, &event, 1)) {
        sk_print("clean up skio event from output queue\n");
        _event_destroy(&event);
    }
}

// APIs
sk_sched_t* sk_sched_create(void* evlp, sk_entity_mgr_t* entity_mgr, int flags)
{
    sk_sched_t* sched = calloc(1, sizeof(*sched) +
                               SK_EVENT_SZ * SK_SCHED_PULL_NUM);
    sched->evlp       = evlp;
    sched->entity_mgr = entity_mgr;
    sched->pto_tbl    = sk_pto_tbl;
    sched->running    = 0;
    sched->flags      = flags;

    // create the default io
    for (int i = SK_PTO_PRI_MIN; i <= SK_PTO_PRI_MAX; i++) {
        sched->io_tbl[i] = sk_io_create(0, 0);
    }

    _check_ptos(sched->pto_tbl);
    return sched;
}

void sk_sched_destroy(sk_sched_t* sched)
{
    // 1. destroy io bridge
    for (uint32_t bri_idx = 0; bri_idx < sched->bridge_size; bri_idx++) {
        sk_io_bridge_destroy(sched->bridge_tbl[bri_idx]);
    }

    // 2. destroy io
    for (int i = 0; i < SK_PTO_PRI_SZ; i++) {
        // 2.1 clean up the events data first
        _cleanup_skio_events(sched->io_tbl[i]);

        // 2.2 destroy the sk_io
        sk_io_destroy(sched->io_tbl[i]);
    }

    free(sched);
}

void sk_sched_start(sk_sched_t* sched)
{
    sched->running = 1;

    do {
        // here, all the active events in all the priority queues should be
        // executed, or the event may be delayed for a long time
        _process_events(sched);

        // pull all io events and convert them to sched events
        sk_eventloop_dispatch(sched->evlp, 1000);
    } while (sched->running);
}

void sk_sched_stop(sk_sched_t* sched)
{
    sched->running = 0;
}

int sk_sched_setup_bridge(sk_sched_t* src, sk_sched_t* dst)
{
    // register a sk_io which is only used for delivery message in dst side
    if (src->bridge_size == SK_SCHED_MAX_IO_BRIDGE) {
        return 1;
    }

    sk_io_bridge_t* io_bridge = sk_io_bridge_create(dst, 65535);
    src->bridge_tbl[src->bridge_size++] = io_bridge;
    return 0;
}

int sk_sched_send(sk_sched_t* sched, sk_sched_t* dst,
                  sk_entity_t* entity, sk_txn_t* txn,
                  uint32_t pto_id, void* proto_msg, int flag)
{
    SK_ASSERT(entity);
    sk_io_type_t io_type = sched == dst ? SK_IO_INPUT : SK_IO_OUTPUT;

    return _emit_event(sched, dst, io_type, entity, txn, pto_id, proto_msg, flag);
}
