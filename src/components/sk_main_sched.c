#include <stdlib.h>

#include "api/sk_assert.h"
#include "api/sk_sched.h"

// APIs
sk_sched_t* sk_main_sched_create(void* evlp, int strategy)
{
    sk_sched_opt_t opt;
    opt.pto_tbl = NULL;

    return sk_sched_create(evlp, opt);
}
