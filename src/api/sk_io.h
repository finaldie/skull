// sk_io is a message queue which controlled by scheduler.
// sk_io have two message queues: one is input queue, the other is output queue.
//   - The input queue can be triggered by system or user.
//   - The output queue is used for transmit an event to other sk_io's input
//     queue.

#ifndef SK_IO_H
#define SK_IO_H

#include "sk_types.h"
#include "sk_event.h"

typedef struct sk_io_t sk_io_t;

sk_io_t* sk_io_create(size_t input_sz, size_t output_sz);
void sk_io_destroy(sk_io_t* io);

typedef enum sk_io_type_t {
    SK_IO_INPUT = 0,
    SK_IO_OUTPUT,
} sk_io_type_t;

// push N events into sk_io input or output queue
void sk_io_push(sk_io_t* io, sk_io_type_t type, sk_event_t* events,
                size_t nevents);

// try to pull N events from sk_io (input or output) queue
// return the actual event count
size_t sk_io_pull(sk_io_t* io, sk_io_type_t type, sk_event_t* events,
                  size_t nevents);

size_t sk_io_used(sk_io_t* io, sk_io_type_t type);
size_t sk_io_free(sk_io_t* io, sk_io_type_t type);

#endif

