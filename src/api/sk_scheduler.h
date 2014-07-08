#ifndef SK_SCHEDULER_H
#define SK_SCHEDULER_H

#include "sk_io.h"

typedef struct sk_scheduler_t sk_scheduler_t;

sk_scheduler_t* sk_scheduler_create(sk_io_t** io_tbl);
void sk_scheduler_destroy(sk_scheduler_t* scheduler);

void sk_scheduler_start(sk_scheduler_t* scheduler);
void sk_scheduler_stop(sk_scheduler_t* scheduler);

#endif

