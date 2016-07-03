#ifndef SK_MBUF_H
#define SK_MBUF_H

/**
 * A fifo memory array buffer
 */

#include <stddef.h>

typedef struct sk_mbuf_t sk_mbuf_t;

sk_mbuf_t* sk_mbuf_create(size_t init_size, size_t max_size);
void sk_mbuf_destroy(sk_mbuf_t*);

int sk_mbuf_push(sk_mbuf_t*, const void* data, size_t size);
int sk_mbuf_pop(sk_mbuf_t*, void* data, size_t size);

void* sk_mbuf_rawget(sk_mbuf_t*, size_t size);
size_t sk_mbuf_size(sk_mbuf_t*);

#endif

