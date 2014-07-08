#ifndef SK_IO_H
#define SK_IO_H

#include "sk_event.h"

typedef struct sk_io_t {
    void* data;

    void (*init)(void* data);
    void (*destroy)(void* data);
    void (*open)(void* data);
    void (*close)(void* data);
    int (*pull)(void* data, sk_event_t* events, int nevent);
} sk_io_t;

extern sk_io_t* sk_io_tbl[];

#endif

