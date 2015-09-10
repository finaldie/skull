#include <stdlib.h>

#include "flibs/fmbuf.h"
#include "api/sk_utils.h"
#include "api/sk_queue.h"

#define SK_QUEUE_ELEM_ANY (-1)
#define SK_QUEUE_DEFAULT_WRITE_SLOTS (1)

typedef struct sk_queue_opt_t {
    void (*create)  (sk_queue_t*, size_t elem_sz, size_t init_slots);
    void (*destroy) (sk_queue_t*);

    int    (*push) (sk_queue_t*, const sk_queue_elem_base_t* elem);
    size_t (*pull) (sk_queue_t*, sk_queue_elem_base_t* elems, size_t max_slots);
} sk_queue_opt_t;

struct sk_queue_t {
    sk_queue_mode_t  mode;
    sk_queue_state_t state;

    sk_queue_opt_t opt;
    size_t elem_sz;
    size_t max_slots;

    union {
        fmbuf* mq; // exclusive or rw (prefer write)

        struct rw { // rw (prefer read)
            fmbuf* rmq;
            fmbuf* wmq;
        } rw;
    } data;
};

// Internal APIs
static
size_t _get_new_size(fmbuf* mq,
                     size_t new_slots, size_t max_slots, size_t elem_sz)
{
    size_t curr_sz = fmbuf_size(mq);
    size_t used_sz = fmbuf_used(mq);
    size_t new_sz = curr_sz ? curr_sz : 1;

    while (new_sz > 0) {
        new_sz *= 2;

        if (new_sz - used_sz >= new_slots * elem_sz) {
            break;
        }
    }

    size_t max_buf_sz = max_slots * elem_sz;
    new_sz = new_sz > max_buf_sz ? max_buf_sz : new_sz;
    SK_ASSERT(new_sz > 0);
    return new_sz;
}

static
int _sk_queue_push(fmbuf** raw_mq, size_t max_slots,
                   const sk_queue_elem_base_t* elem, size_t elem_sz)
{
    fmbuf* mq = *raw_mq;
    int ret = fmbuf_push(mq, elem, elem_sz);
    if (!ret) {
        return 0;
    }

    size_t current_sz = fmbuf_size(mq);
    size_t new_sz = _get_new_size(mq, 1, max_slots, elem_sz);
    if (current_sz == new_sz) {
        return 1;
    }

    *raw_mq = fmbuf_realloc(mq, new_sz);
    ret = fmbuf_push(*raw_mq, elem, elem_sz);
    SK_ASSERT(!ret);

    return 0;
}

static
size_t _sk_queue_pull(fmbuf* mq, int type, size_t elem_sz,
                      sk_queue_elem_base_t* elems, size_t max_slots)
{
    char* tmp_elem[elem_sz];
    sk_queue_elem_base_t* elem = NULL;
    size_t pulled_cnt = 0;

    while (pulled_cnt < max_slots) {
        elem = fmbuf_rawget(mq, &tmp_elem, elem_sz);

        if (!elem) {
            break;
        }

        // found a un-expected task, stop it
        if (SK_QUEUE_ELEM_ANY != type && (int)elem->type != type) {
            break;
        }

        int ret = fmbuf_pop(mq, elems + pulled_cnt * elem_sz, elem_sz);
        SK_ASSERT(!ret);
        pulled_cnt++;
    };

    return pulled_cnt;
}

//// Exclusive
static
void _sk_queue_create_exclusive(sk_queue_t* queue,
                                size_t elem_sz, size_t init_slots)
{
    queue->data.mq = fmbuf_create(elem_sz * init_slots);
}

static
void _sk_queue_destroy_exlusive(sk_queue_t* queue)
{
    fmbuf_delete(queue->data.mq);
}

static
int _sk_queue_push_exlusive(sk_queue_t* queue, const sk_queue_elem_base_t* elem)
{
    SK_ASSERT_MSG(elem->type == SK_QUEUE_ELEM_EXCLUSIVE,
                  "element type is invalid %d\n", elem->type);

    return _sk_queue_push(&queue->data.mq, queue->max_slots,
                          elem, queue->elem_sz);
}

// for exlusive queue, it at most can pull one element in one time
// return how many elements it pulled
static
size_t _sk_queue_pull_exlusive(sk_queue_t* queue,
                               sk_queue_elem_base_t* elems, size_t max_slots)
{
    if (queue->state == SK_QUEUE_STATE_LOCK) {
        return 0;
    }

    fmbuf* mq = queue->data.mq;
    int ret = fmbuf_pop(mq, elems, queue->elem_sz);
    if (!ret) {
        return 1;
    } else {
        return 0;
    }
}

static
sk_queue_opt_t _sk_queue_opt_exlusive = {
    .create  = _sk_queue_create_exclusive,
    .destroy = _sk_queue_destroy_exlusive,
    .push = _sk_queue_push_exlusive,
    .pull = _sk_queue_pull_exlusive
};

//// Read-Write (prefer write)
static
void _sk_queue_create_rw_pw(sk_queue_t* queue,
                            size_t elem_sz, size_t init_slots)
{
    _sk_queue_create_exclusive(queue, elem_sz, init_slots);
}

static
void _sk_queue_destroy_rw_pw(sk_queue_t* queue)
{
    _sk_queue_destroy_exlusive(queue);
}

static
int _sk_queue_push_rw_pw(sk_queue_t* queue, const sk_queue_elem_base_t* elem)
{
    SK_ASSERT_MSG(elem->type == SK_QUEUE_ELEM_READ ||
                  elem->type == SK_QUEUE_ELEM_WRITE,
                  "element type is invalid %d\n", elem->type);

    return _sk_queue_push(&queue->data.mq, queue->max_slots,
                          elem, queue->elem_sz);
}

static
size_t _sk_queue_pull_rw_pw(sk_queue_t* queue,
                         sk_queue_elem_base_t* elems, size_t max_slots)
{
    size_t pulled_cnt = 0;
    fmbuf* mq = queue->data.mq;

    if (queue->state == SK_QUEUE_STATE_IDLE) {
        // pull one elem first
        pulled_cnt = _sk_queue_pull(mq, SK_QUEUE_ELEM_ANY,
                                    queue->elem_sz, elems, 1);

        if (pulled_cnt == 0) {
            return 0;
        }

        if (max_slots == 1) {
            return 1;
        }

        if (elems->type == SK_QUEUE_ELEM_WRITE) {
            return 1;
        }

        // now, let's pull the read elements as much as possible
        SK_ASSERT(elems->type == SK_QUEUE_ELEM_READ);
        pulled_cnt +=
            _sk_queue_pull(mq, elems->type, queue->elem_sz,
                           elems + pulled_cnt * queue->elem_sz, max_slots - 1);

    } else if (queue->state == SK_QUEUE_STATE_READ) {
        pulled_cnt = _sk_queue_pull(mq, SK_QUEUE_ELEM_READ, queue->elem_sz,
                                    elems, max_slots);

    } else if (queue->state == SK_QUEUE_STATE_WRITE) {
        pulled_cnt = 0;

    } else {
        SK_ASSERT(0);
    }

    return pulled_cnt;
}

static
sk_queue_opt_t _sk_queue_opt_rw_pw = {
    .create  = _sk_queue_create_rw_pw,
    .destroy = _sk_queue_destroy_rw_pw,
    .push = _sk_queue_push_rw_pw,
    .pull = _sk_queue_pull_rw_pw
};

//// Read-Write (prefer read)
static
void _sk_queue_create_rw_pr(sk_queue_t* queue,
                            size_t elem_sz, size_t init_slots)
{
    queue->data.rw.rmq = fmbuf_create(elem_sz * init_slots);
    queue->data.rw.wmq = fmbuf_create(elem_sz * SK_QUEUE_DEFAULT_WRITE_SLOTS);
}

static
void _sk_queue_destroy_rw_pr(sk_queue_t* queue)
{
    fmbuf_delete(queue->data.rw.rmq);
    fmbuf_delete(queue->data.rw.wmq);
}

static
int _sk_queue_push_rw_pr(sk_queue_t* queue, const sk_queue_elem_base_t* elem)
{
    SK_ASSERT_MSG(elem->type == SK_QUEUE_ELEM_READ ||
                  elem->type == SK_QUEUE_ELEM_WRITE,
                  "element type is invalid %d\n", elem->type);

    fmbuf** mq = NULL;
    if (elem->type == SK_QUEUE_ELEM_READ) {
        mq = &queue->data.rw.rmq;
    } else {
        mq = &queue->data.rw.wmq;
    }

    return _sk_queue_push(mq, queue->max_slots, elem, queue->elem_sz);
}

static
size_t _sk_queue_pull_rw_pr(sk_queue_t* queue,
                            sk_queue_elem_base_t* elems, size_t max_slots)
{
    fmbuf* rmq = queue->data.rw.rmq;
    fmbuf* wmq = queue->data.rw.wmq;
    size_t pulled_cnt = 0;

    // 1. always try to pull the write queue first
    if (fmbuf_used(wmq) > 0) {
        pulled_cnt = _sk_queue_pull(wmq, SK_QUEUE_ELEM_WRITE, queue->elem_sz,
                                    elems, 1);

        if (pulled_cnt > 0) {
            SK_ASSERT(pulled_cnt == 1);
            return 1;
        }
    }

    // 2. Thre is no write element be pulled, try to pull read elements
    SK_ASSERT(pulled_cnt == 0);
    return _sk_queue_pull(rmq, SK_QUEUE_ELEM_READ, queue->elem_sz,
                          elems, max_slots);
}

static
sk_queue_opt_t _sk_queue_opt_rw_pr = {
    .create  = _sk_queue_create_rw_pr,
    .destroy = _sk_queue_destroy_rw_pr,
    .push = _sk_queue_push_rw_pr,
    .pull = _sk_queue_pull_rw_pr
};

// Public APIs
sk_queue_t* sk_queue_create(sk_queue_mode_t mode, size_t elem_sz,
                            size_t init_slots, size_t max_slots)
{
    if (max_slots == 0 || init_slots > max_slots) {
        return NULL;
    }

    SK_ASSERT_MSG(elem_sz * max_slots / max_slots == elem_sz,
                  "sk_queue: error! no enough space for creating the queue\n");

    sk_queue_t* queue = calloc(1, sizeof(*queue));
    queue->mode  = mode;
    queue->state = SK_QUEUE_STATE_IDLE;
    queue->elem_sz   = elem_sz;
    queue->max_slots = max_slots;

    switch (mode) {
    case SK_QUEUE_EXCLUSIVE:
        queue->opt = _sk_queue_opt_exlusive;
        break;
    case SK_QUEUE_RW_PR:
        queue->opt = _sk_queue_opt_rw_pw;
        break;
    case SK_QUEUE_RW_PW:
        queue->opt = _sk_queue_opt_rw_pr;
        break;
    default:
        SK_ASSERT(0);
    }

    queue->opt.create(queue, elem_sz, init_slots);

    return queue;
}

void sk_queue_destroy(sk_queue_t* queue)
{
    if (!queue) {
        return;
    }

    queue->opt.destroy(queue);
    free(queue);
}

int sk_queue_push(sk_queue_t* queue, const sk_queue_elem_base_t* elem)
{
    SK_ASSERT(queue);
    SK_ASSERT(elem);

    return queue->opt.push(queue, elem);
}

size_t sk_queue_pull(sk_queue_t* queue,
                     sk_queue_elem_base_t* elems, size_t max_slots)
{
    SK_ASSERT(queue);
    SK_ASSERT(elems);
    SK_ASSERT(max_slots);

    size_t max_pull_slots = max_slots;
    if (max_slots > queue->max_slots) {
        max_pull_slots = queue->max_slots;
    }

    return queue->opt.pull(queue, elems, max_pull_slots);
}

sk_queue_state_t sk_queue_state(sk_queue_t* queue)
{
    SK_ASSERT(queue);
    return queue->state;
}

void sk_queue_setstate(sk_queue_t* queue, sk_queue_state_t state)
{
    SK_ASSERT(queue);

    switch (queue->mode) {
    case SK_QUEUE_EXCLUSIVE:
    {
        if (state != SK_QUEUE_STATE_IDLE && state != SK_QUEUE_STATE_LOCK) {
            SK_ASSERT(0);
        }

        queue->state = state;
        break;
    }
    case SK_QUEUE_RW_PR:
    case SK_QUEUE_RW_PW:
        if (state == SK_QUEUE_STATE_LOCK) {
            SK_ASSERT(0);
        }

        break;
    default:
        SK_ASSERT(0);
    }
}

bool sk_queue_empty(sk_queue_t* queue)
{
    SK_ASSERT(queue);

    switch (queue->mode) {
    case SK_QUEUE_EXCLUSIVE:
    case SK_QUEUE_RW_PW:
    {
        return fmbuf_used(queue->data.mq) == 0;
    }
    case SK_QUEUE_RW_PR:
        return (fmbuf_used(queue->data.rw.rmq) == 0) &&
               (fmbuf_used(queue->data.rw.wmq) == 0);
    default:
        SK_ASSERT(0);
        return true;
    }
}
