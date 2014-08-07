#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "fmbuf/fmbuf.h"
#include "api/sk_utils.h"
#include "api/sk_io.h"

struct sk_io_t {
    fmbuf* mq[2];
};

static
int _get_new_size(fmbuf* mq, int nevents)
{
    int curr_sz = fmbuf_size(mq);
    int used_sz = fmbuf_used(mq);
    int new_sz = curr_sz ? curr_sz : 1;

    while (new_sz > 0) {
        new_sz *= 2;
        if (new_sz - used_sz >= nevents * (int)SK_EVENT_SZ) {
            break;
        }
    }

    SK_ASSERT(new_sz != 0);
    return new_sz > 0 ? new_sz : INT32_MAX;
}

sk_io_t* sk_io_create(int input_sz, int output_sz)
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

void sk_io_push(sk_io_t* io, int type, sk_event_t* events, int nevents)
{
    fmbuf* mq = io->mq[type];
    int ret = fmbuf_push(mq, events, SK_EVENT_SZ * nevents);
    if (!ret) {
        // push done, return directly
        return;
    }

    int new_sz = _get_new_size(mq, nevents);
    printf("new_sz = %d\n", new_sz);
    io->mq[type] = fmbuf_realloc(mq, new_sz);
    mq = io->mq[type];
    ret = fmbuf_push(mq, events, SK_EVENT_SZ * nevents);
    SK_ASSERT(!ret);
}

int sk_io_pull(sk_io_t* io, int type, sk_event_t* events, int nevents)
{
    fmbuf* mq = io->mq[type];
    int mq_cnt = fmbuf_used(mq) / SK_EVENT_SZ;
    int actual_cnt = nevents > mq_cnt ? mq_cnt : nevents;

    if (mq_cnt == 0) {
        return 0;
    }

    int actual_sz = actual_cnt * SK_EVENT_SZ;
    int ret = fmbuf_pop(mq, events, actual_sz);
    SK_ASSERT(!ret);
    return actual_cnt;
}

int sk_io_used(sk_io_t* io, int type)
{
    fmbuf* mq = io->mq[type];
    return fmbuf_used(mq) / SK_EVENT_SZ;
}

int sk_io_free(sk_io_t* io, int type)
{
    fmbuf* mq = io->mq[type];
    return fmbuf_free(mq) / SK_EVENT_SZ;
}
