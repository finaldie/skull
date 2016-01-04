#ifndef SK_ENTITY_H
#define SK_ENTITY_H

#include <stddef.h>
#include <unistd.h>
#include <stdint.h>

#include "api/sk_object.h"

typedef enum sk_entity_type_t {
    SK_ENTITY_NONE = 0,
    SK_ENTITY_NET  = 1,
    SK_ENTITY_STD  = 2
} sk_entity_type_t;

typedef enum sk_entity_status_t {
    SK_ENTITY_ACTIVE = 0, // read to do the task
    SK_ENTITY_INACTIVE,   // ready to be deleted
    SK_ENTITY_DEAD
} sk_entity_status_t;

struct sk_entity_mgr_t;
struct sk_workflow_t;
struct sk_txn_t;

typedef struct sk_entity_t sk_entity_t;

typedef struct sk_entity_opt_t {
    ssize_t (*read)    (sk_entity_t*, void* buf, size_t len, void* ud);
    ssize_t (*write)   (sk_entity_t*, const void* buf, size_t len, void* ud);
    void    (*destroy) (sk_entity_t*, void* ud);
} sk_entity_opt_t;

sk_entity_t* sk_entity_create(struct sk_workflow_t* workflow);
void sk_entity_destroy(sk_entity_t* entity);

ssize_t sk_entity_read(sk_entity_t* entity, void* buf, size_t buf_len);
ssize_t sk_entity_write(sk_entity_t* entity, const void* buf, size_t buf_len);

sk_entity_type_t sk_entity_type(sk_entity_t* entity);
sk_entity_status_t sk_entity_status(sk_entity_t* entity);
struct sk_workflow_t* sk_entity_workflow(sk_entity_t* entity);

void sk_entity_setopt(sk_entity_t* entity, sk_entity_opt_t opt, void* ud);
void sk_entity_mark(sk_entity_t* entity, sk_entity_status_t status);

struct sk_entity_mgr_t* sk_entity_owner(sk_entity_t* entity);
void sk_entity_setowner(sk_entity_t* entity, struct sk_entity_mgr_t* mgr);

struct sk_txn_t* sk_entity_halftxn(sk_entity_t* entity);
void sk_entity_sethalftxn(sk_entity_t* entity, struct sk_txn_t* txn);

void sk_entity_txnadd(sk_entity_t*, const struct sk_txn_t*);
void sk_entity_txndel(sk_entity_t*, const struct sk_txn_t*);
int  sk_entity_taskcnt(sk_entity_t* entity);

void sk_entity_timeradd(sk_entity_t*, const sk_obj_t*);
void sk_entity_timerdel(sk_entity_t*, const sk_obj_t*);

// create network entity from a base entity
void sk_entity_net_create(sk_entity_t* entity, void* ud);
void sk_entity_stdin_create(sk_entity_t* entity, void* ud);

#endif

