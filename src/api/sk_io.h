#ifndef SK_IO_H
#define SK_IO_H

#include <stddef.h>
#include "sk_types.h"
#include "sk_event.h"

#define SK_IO_TIMER      0
#define SK_IO_NET_SOCK   1
#define SK_IO_PIPE       2
#define SK_IO_NET_ACCEPT 3

typedef struct sk_io_t sk_io_t;

typedef struct sk_io_opt_t {
    void* data;

    int  (*open)  (sk_io_t*, void* data);
    void (*close) (sk_io_t*, void* data);
    int  (*pull)  (sk_io_t*, void* data, sk_event_t* events, int nevents);
    int  (*reg)   (sk_io_t*, void* data, sk_event_t* event/*out*/);
} sk_io_opt_t;

sk_io_t* sk_io_create(void* evlp, int type, sk_io_opt_t opt);
void sk_io_destroy(sk_io_t* io);

int sk_io_register(sk_io_t* io, void* data, sk_event_t* event);
int sk_io_push(sk_io_t* io, sk_event_t* event);
int sk_io_pull(sk_io_t* io, sk_event_t* events, int nevents);

#endif

