#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "flibs/fev_buff.h"

#include "api/sk_utils.h"
#include "api/sk_entity.h"

sk_entity_opt_t sk_entity_stdin_opt;

typedef struct sk_entity_stdin_data_t {
    fev_buff* evbuff;
} sk_entity_stdin_data_t;

void sk_entity_stdin_create(sk_entity_t* entity, void* evbuff)
{
    sk_entity_stdin_data_t* data = calloc(1, sizeof(*data));
    data->evbuff = evbuff;

    sk_entity_setopt(entity, sk_entity_stdin_opt, data);
}

static
ssize_t _stdin_read(sk_entity_t* entity, void* buf, size_t len, void* ud)
{
    if (!ud) return -1;

    sk_entity_stdin_data_t* data = ud;
    fev_buff* evbuff = data->evbuff;

    return fevbuff_read(evbuff, buf, len);
}

static
ssize_t _stdout_write(sk_entity_t* entity, const void* buf, size_t len,
                      void* ud)
{
    int printed = fprintf(stdout, "%s", (const char*) buf);
    SK_ASSERT_MSG(len == (size_t)printed, "expect print len: %zu, real %d\n",
                  len, printed);

    if (!fflush(stdout)) {
        return -1;
    }

    return printed;
}

static
void _std_destroy(sk_entity_t* entity, void* ud)
{
    if (!ud) return;
    sk_print("stdin entity destroy\n");

    sk_entity_stdin_data_t* data = ud;
    fev_buff* evbuff = data->evbuff;
    fevbuff_destroy(evbuff);

    free(data);
}

static
void* _std_rbufget(const sk_entity_t* entity, void* ud)
{
    sk_entity_stdin_data_t* data = ud;
    fev_buff* evbuff = data->evbuff;

    return fevbuff_rawget(evbuff);
}

static
size_t _std_rbufsz(const sk_entity_t* entity, void* ud)
{
    sk_entity_stdin_data_t* data = ud;
    fev_buff* evbuff = data->evbuff;

    return fevbuff_get_usedlen(evbuff, FEVBUFF_TYPE_READ);
}

static
size_t _std_rbufpop(sk_entity_t* entity, size_t popsz, void* ud)
{
    sk_entity_stdin_data_t* data = ud;
    fev_buff* evbuff = data->evbuff;

    return fevbuff_pop(evbuff, popsz);
}

sk_entity_opt_t sk_entity_stdin_opt = {
    .read    = _stdin_read,
    .write   = _stdout_write,
    .destroy = _std_destroy,

    .rbufget = _std_rbufget,
    .rbufsz  = _std_rbufsz,
    .rbufpop = _std_rbufpop,
    .peer    = NULL
};
