#ifndef SK_EVENTLOOP_H
#define SK_EVENTLOOP_H

void* sk_eventloop_create();
void sk_eventloop_destroy(void* evlp);

// return - > 0: how many event have been processed
//        - <= 0: no event has been processed
int sk_eventloop_dispatch(void* evlp, int timeout/*ms*/);

#endif

