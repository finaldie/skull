#ifndef SK_TASK_QUEUE_H
#define SK_TASK_QUEUE_H

#include <stddef.h>
#include <stdbool.h>

#define SK_QUEUE_ELEM(elem)  ((sk_queue_elem_base_t*)elem)

typedef enum sk_queue_mode_t {
    SK_QUEUE_EXCLUSIVE = 0,
    SK_QUEUE_RW_PR = 1,
    SK_QUEUE_RW_PW = 2
} sk_queue_mode_t;

typedef enum sk_queue_state_t {
    SK_QUEUE_STATE_INIT    = 0,
    SK_QUEUE_STATE_DESTROY = 1,
    SK_QUEUE_STATE_IDLE    = 2,
    SK_QUEUE_STATE_LOCK    = 3,    // for exlusive queue
    SK_QUEUE_STATE_READ    = 4,    // for read-write queue
    SK_QUEUE_STATE_WRITE   = 5     // for reqd-write queue
} sk_queue_state_t;

typedef enum sk_queue_elem_type_t {
    SK_QUEUE_ELEM_EXCLUSIVE = 0,
    SK_QUEUE_ELEM_READ  = 1,
    SK_QUEUE_ELEM_WRITE = 2
} sk_queue_elem_type_t;

// Every element in the sk_queue must with this 'elem_base' in its head
typedef struct sk_queue_elem_base_t {
    sk_queue_elem_type_t type;
} sk_queue_elem_base_t;

typedef struct sk_queue_t sk_queue_t;

sk_queue_t* sk_queue_create(sk_queue_mode_t mode, size_t element_sz,
                            size_t init_slots, size_t max_slots);
void sk_queue_destroy(sk_queue_t*);

int sk_queue_push(sk_queue_t*, const sk_queue_elem_base_t* element);
size_t sk_queue_pull(sk_queue_t*, sk_queue_elem_base_t* elements,
                     size_t max_slots);

sk_queue_state_t sk_queue_state(sk_queue_t*);
void sk_queue_setstate(sk_queue_t*, sk_queue_state_t);

bool sk_queue_empty(sk_queue_t*);

#endif

