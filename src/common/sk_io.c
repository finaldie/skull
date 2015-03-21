#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "flibs/fmbuf.h"
#include "api/sk_utils.h"
#include "api/sk_io.h"

struct sk_io_t {
    fmbuf* mq[2];
};

static
size_t _get_new_size(fmbuf* mq, size_t nevents)
{
    size_t curr_cnt = fmbuf_size(mq) / SK_EVENT_SZ;
    size_t used_cnt = fmbuf_used(mq) / SK_EVENT_SZ;
    size_t new_cnt = curr_cnt ? curr_cnt : 1;
    size_t new_sz = new_cnt * SK_EVENT_SZ;

    while (new_sz > 0) {
        new_sz *= 2;

        if (new_sz - (used_cnt * SK_EVENT_SZ) >= nevents * SK_EVENT_SZ) {
            break;
        }
    }

    SK_ASSERT(new_sz > 0);
    return new_sz;
}

sk_io_t* sk_io_create(size_t input_sz, size_t output_sz)
{
    sk_io_t* io = malloc(sizeof(*io));
    io->mq[0] = fmbuf_create(SK_EVENT_SZ * input_sz); // SK_IO_INPUT
    io->mq[1] = fmbuf_create(SK_EVENT_SZ * output_sz); // SK_IO_OUTPUT

    return io;
}

void sk_io_destroy(sk_io_t* io)
{
    fmbuf_delete(io->mq[0]);
    fmbuf_delete(io->mq[1]);
    free(io);
}

void sk_io_push(sk_io_t* io, sk_io_type_t type, sk_event_t* events,
                size_t nevents)
{
    fmbuf* mq = io->mq[type];
    int ret = fmbuf_push(mq, events, SK_EVENT_SZ * (size_t)nevents);
    if (!ret) {
        // push done, return directly
        return;
    }

    size_t new_sz = _get_new_size(mq, nevents);
    sk_print("new_sz = %zu\n", new_sz);
    io->mq[type] = fmbuf_realloc(mq, new_sz);
    mq = io->mq[type];
    ret = fmbuf_push(mq, events, SK_EVENT_SZ * (size_t)nevents);
    SK_ASSERT(!ret);
}

size_t sk_io_pull(sk_io_t* io, sk_io_type_t type, sk_event_t* events,
                  size_t nevents)
{
    fmbuf* mq = io->mq[type];
    size_t mq_cnt = fmbuf_used(mq) / SK_EVENT_SZ;
    size_t actual_cnt = (size_t)nevents > mq_cnt ? mq_cnt : nevents;

    if (mq_cnt == 0) {
        return 0;
    }

    size_t actual_sz = actual_cnt * SK_EVENT_SZ;
    int ret = fmbuf_pop(mq, events, actual_sz);
    SK_ASSERT(!ret);
    return actual_cnt;
}

size_t sk_io_used(sk_io_t* io, sk_io_type_t type)
{
    fmbuf* mq = io->mq[type];
    return fmbuf_used(mq) / SK_EVENT_SZ;
}

size_t sk_io_free(sk_io_t* io, sk_io_type_t type)
{
    fmbuf* mq = io->mq[type];
    return fmbuf_free(mq) / SK_EVENT_SZ;
}

sk_event_t* sk_io_rawget(sk_io_t* io, sk_io_type_t type)
{
    fmbuf* mq = io->mq[type];
    if (fmbuf_used(mq) == 0) {
        return NULL;
    }

    sk_event_t tmp;
    sk_event_t* event = fmbuf_rawget(mq, &tmp, SK_EVENT_SZ);
    SK_ASSERT(event);
    return event;
}
