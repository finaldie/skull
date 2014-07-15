#include <stdlib.h>

#include "fmbuf/fmbuf.h"
#include "api/sk_assert.h"
#include "api/sk_io.h"

struct sk_io_t {
    fmbuf* mq[2];
};


sk_io_t* sk_io_create(int size)
{
    sk_io_t* io = malloc(sizeof(*io));
    io->mq[0] = fmbuf_create(SK_EVENT_SZ * size);
    io->mq[1] = fmbuf_create(SK_EVENT_SZ * size);

    return io;
}

void sk_io_destroy(sk_io_t* io)
{
    fmbuf_delete(io->mq[0]);
    fmbuf_delete(io->mq[1]);
    free(io);
}

int sk_io_push(sk_io_t* io, int type, sk_event_t* events, int nevents)
{
    fmbuf* mq = io->mq[type];
    return fmbuf_push(mq, events, SK_EVENT_SZ * nevents);
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
