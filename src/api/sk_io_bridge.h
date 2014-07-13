#ifndef SK_IO_BRIDGE_H
#define SK_IO_BRIDGE_H

#include "api/sk_io.h"

typedef struct sk_io_bridge_t sk_io_bridge_t;

sk_io_bridge_t* sk_io_bridge_create(sk_io_t* src_io, sk_io_t* dst_io,
                                    void* dst_evlp, int size);
void sk_io_bridge_destroy(sk_io_bridge_t* io_bridge);
void sk_io_bridge_deliver(sk_io_bridge_t* io_bridge);

#endif

