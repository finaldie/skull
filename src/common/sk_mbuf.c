#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "flibs/fmbuf.h"
#include "api/sk_mbuf.h"

struct sk_mbuf_t {
    fmbuf* buf;
    size_t max_sz;
};

sk_mbuf_t* sk_mbuf_create(size_t init_size, size_t max_size) {
    sk_mbuf_t* buf = calloc(1, sizeof(*buf));
    buf->buf = fmbuf_create(init_size);
    buf->max_sz = max_size;
    return buf;
}

void sk_mbuf_destroy(sk_mbuf_t* buf) {
    if (!buf) return;

    fmbuf_delete(buf->buf);
    free(buf);
}

int sk_mbuf_push(sk_mbuf_t* buf, const void* data, size_t size) {
    // 1. Push directly
    int ret = fmbuf_push(buf->buf, data, size);
    if (!ret) {
        return 0;
    }

    // 2. Realloc it and push it again
    //  2.1 Calculate the reallocation size
    size_t curr_used = fmbuf_used(buf->buf);
    size_t curr_size = fmbuf_size(buf->buf);
    size_t free_size = fmbuf_free(buf->buf);
    bool loop_again = true;

    size_t max_size = buf->max_sz ? buf->max_sz : SIZE_MAX;
    size_t new_size = curr_size ? curr_size : 1;

    while (free_size < size && loop_again) {
        if (new_size <= SIZE_MAX / 2) {
            new_size *= 2;
        } else {
            new_size = max_size;
            loop_again = false;
        }

        free_size = new_size - curr_used;
    }

    // 2.2 Realloc it and push it again
    buf->buf = fmbuf_realloc(buf->buf, new_size);
    return fmbuf_push(buf->buf, data, size);
}

int sk_mbuf_pop(sk_mbuf_t* buf, void* data, size_t size) {
    return fmbuf_pop(buf->buf, data, size);
}

void* sk_mbuf_rawget(sk_mbuf_t* buf, size_t size) {
    size_t curr_used = fmbuf_used(buf->buf);

    if (!size || size <= curr_used) {
        return fmbuf_head(buf->buf);
    }

    return NULL;
}

size_t sk_mbuf_size(sk_mbuf_t* buf) {
    return fmbuf_used(buf->buf);
}
