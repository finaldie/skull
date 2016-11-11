#ifndef SK_TRIGGER_H
#define SK_TRIGGER_H

#include "api/sk_workflow.h"
#include "api/sk_engine.h"
#include "api/sk_config.h"

typedef enum sk_trigger_type_t {
    SK_TRIGGER_IMMEDIATELY = 0,
    SK_TRIGGER_BY_STDIN,
    SK_TRIGGER_BY_TCP,
    SK_TRIGGER_BY_UDP
} sk_trigger_type_t;

struct sk_trigger_t;
typedef struct sk_trigger_opt_t {
    // init the data field
    void (*create)  (struct sk_trigger_t*);   // create workflow trigger
    void (*run)     (struct sk_trigger_t*);   // run the trigger
    void (*destroy) (struct sk_trigger_t*);   // destroy the data field
} sk_trigger_opt_t;

typedef struct sk_trigger_t {
    sk_trigger_type_t type;

#if __WORDSIZE == 64
    int padding;
#endif

    sk_engine_t*   engine;
    sk_workflow_t* workflow;
    void* data;

    sk_trigger_opt_t opt;
} sk_trigger_t;

sk_trigger_t* sk_trigger_create(sk_engine_t* engine, sk_workflow_t* workflow);

void sk_trigger_destroy(sk_trigger_t* trigger);

void sk_trigger_run(sk_trigger_t* trigger);

#endif

