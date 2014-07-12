#include <stdlib.h>

#include "fmbuf/fmbuf.h"
#include "api/sk_io.h"

struct sk_scheduler_t;

struct sk_io_t {
    struct sk_scheduler_t* owner;
    fmbuf* input;
    fmbuf* output;
    int    type;
#if __WORDSIZE == 64
    int   padding;
#endif

    sk_io_opt_t opt;
};


sk_io_t* sk_io_create(void* evlp, int type, sk_io_opt_t opt)
{
    return NULL;
}

void sk_io_destroy(sk_io_t* io)
{
}

int sk_io_register(sk_io_t* io, void* data, sk_event_t* event)
{
    return 0;
}

int sk_io_push(sk_io_t* io, sk_event_t* event)
{
    return 0;
}

int sk_io_pull(sk_io_t* io, sk_event_t* events, int nevents)
{
    return 0;
}
