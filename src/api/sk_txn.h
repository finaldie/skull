#ifndef SK_TXN_H
#define SK_TXN_H

#include <stddef.h>
#include "flist/flist.h"

struct sk_sched_t;
struct sk_workflow_t;
struct sk_entity_t;
struct sk_module_t;

typedef struct sk_txn_t sk_txn_t;

sk_txn_t* sk_txn_create(struct sk_sched_t* sched,
                        struct sk_workflow_t* workflow,
                        struct sk_entity_t* entity);
void sk_txn_destroy(sk_txn_t* txn);

const char* sk_txn_input(sk_txn_t* txn, size_t* sz);
void sk_txn_set_input(sk_txn_t* txn, const char* data, size_t sz);

void sk_txn_output_append(sk_txn_t* txn, const char* data, size_t sz);
const char* sk_txn_output(sk_txn_t* txn, size_t* sz);

struct sk_sched_t* sk_txn_sched(sk_txn_t* txn);
struct sk_workflow_t* sk_txn_workflow(sk_txn_t* txn);
struct sk_entity_t* sk_txn_entity(sk_txn_t* txn);

struct sk_module_t* sk_txn_next_module(sk_txn_t* txn);
struct sk_module_t* sk_txn_current_module(sk_txn_t* txn);
int sk_txn_is_first_module(sk_txn_t* txn);
int sk_txn_is_last_module(sk_txn_t* txn);

#endif

