#include <stdlib.h>

#include "flibs/fev.h"
#include "api/sk_const.h"

void* sk_eventloop_create(int max_fds)
{
    int max = max_fds > 0 ? max_fds : SK_EVENTLOOP_MAX_EVENTS;
    return fev_create(max);
}
void sk_eventloop_destroy(void* evlp)
{
    fev_destroy(evlp);
}

int sk_eventloop_dispatch(void* evlp, int timeout)
{
    return fev_poll(evlp, timeout);
}
