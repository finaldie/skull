#ifndef SK_ENTITY_H
#define SK_ENTITY_H

#include <stddef.h>
#include <unistd.h>
#include <stdint.h>
#include <arpa/inet.h>

#include "api/sk_object.h"

typedef enum sk_entity_type_t {
    SK_ENTITY_NONE         = 0,
    SK_ENTITY_STD          = 1,
    SK_ENTITY_SOCK_V4TCP   = 2,
    SK_ENTITY_SOCK_V4UDP   = 3,
    SK_ENTITY_SOCK_V6TCP   = 4,
    SK_ENTITY_SOCK_V6UDP   = 5,
    SK_ENTITY_TIMER        = 6,
    SK_ENTITY_EP_V4TCP     = 7,
    SK_ENTITY_EP_V4UDP     = 8,
    SK_ENTITY_EP_V6TCP     = 9,
    SK_ENTITY_EP_V6UDP     = 10,
    SK_ENTITY_EP_TIMER     = 11,
    SK_ENTITY_EP_TXN_TIMER = 12
} sk_entity_type_t;

typedef enum sk_entity_status_t {
    SK_ENTITY_ACTIVE    = 0, // Read to do the task
    SK_ENTITY_INACTIVE,      // Ready to be deleted
    SK_ENTITY_DEAD
} sk_entity_status_t;

struct sk_entity_mgr_t;
struct sk_workflow_t;
struct sk_txn_t;

typedef struct sk_entity_t sk_entity_t;

typedef struct sk_entity_peer_t {
    int  port;
    int  family;
    char name[INET6_ADDRSTRLEN];
    short __padding;
} sk_entity_peer_t;

typedef struct sk_entity_opt_t {
    ssize_t (*read)    (sk_entity_t*, void* buf, size_t len, void* ud);
    ssize_t (*write)   (sk_entity_t*, const void* buf, size_t len, void* ud);
    void    (*destroy) (sk_entity_t*, void* ud);

    void*   (*rbufget) (const sk_entity_t*, void* ud);
    size_t  (*rbufsz)  (const sk_entity_t*, void* ud);
    size_t  (*rbufpop) (sk_entity_t*, size_t popsz, void* ud);

    int     (*peer)    (const sk_entity_t*, sk_entity_peer_t*, void* ud);
} sk_entity_opt_t;

sk_entity_t* sk_entity_create(struct sk_workflow_t* workflow,
                              sk_entity_type_t type);
void sk_entity_destroy(sk_entity_t*);

ssize_t sk_entity_read(sk_entity_t*, void* buf, size_t buf_len);
ssize_t sk_entity_write(sk_entity_t*, const void* buf, size_t buf_len);

void*   sk_entity_rbufget(const sk_entity_t*);
size_t  sk_entity_rbufsz(const sk_entity_t*);
size_t  sk_entity_rbufpop(sk_entity_t*, size_t popsz);
int     sk_entity_peer(const sk_entity_t*, sk_entity_peer_t*);

sk_entity_type_t sk_entity_type(const sk_entity_t*);
sk_entity_status_t sk_entity_status(const sk_entity_t*);
struct sk_workflow_t* sk_entity_workflow(const sk_entity_t*);
int sk_entity_idle(const sk_entity_t*);

void sk_entity_setopt(sk_entity_t*, sk_entity_opt_t opt, void* ud);
void sk_entity_mark(sk_entity_t*, sk_entity_status_t status);

int sk_entity_destroyable(const sk_entity_t*);

// Entity flags
#define SK_ENTITY_F_DESTROY_NOTXN 0x1

void sk_entity_setflags(sk_entity_t*, int flags);
int  sk_entity_flags(const sk_entity_t*);

struct sk_entity_mgr_t* sk_entity_owner(const sk_entity_t*);
void sk_entity_setowner(sk_entity_t*, struct sk_entity_mgr_t* mgr);

struct sk_txn_t* sk_entity_halftxn(const sk_entity_t*);
void sk_entity_sethalftxn(sk_entity_t*, struct sk_txn_t* txn);

struct sk_txn_t* sk_entity_iqueue_pop(sk_entity_t*);
void sk_entity_iqueue_push(sk_entity_t*, const struct sk_txn_t*);

void sk_entity_txnadd(sk_entity_t*, const struct sk_txn_t*);
void sk_entity_txndel(sk_entity_t*, const struct sk_txn_t*);
int  sk_entity_taskcnt(const sk_entity_t*);

void sk_entity_timeradd(sk_entity_t*, const sk_obj_t*);
void sk_entity_timerdel(sk_entity_t*, const sk_obj_t*);

// create an entity from a base entity
void sk_entity_evb_create(sk_entity_t*, void* ud);
void sk_entity_std_create(sk_entity_t*, void* ud);
void sk_entity_tcp_create(sk_entity_t*, void* ud);
void sk_entity_udp_create(sk_entity_t*, int rootfd, const void* buf,
                          uint16_t buf_sz, struct sockaddr* src_addr,
                          socklen_t src_addr_len);

// Debugging API
void sk_entity_dump_txns(const sk_entity_t* entity);

#endif

