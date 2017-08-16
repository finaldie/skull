#ifndef SK_DRIVER_H
#define SK_DRIVER_H

#include "api/sk_workflow.h"
#include "api/sk_engine.h"
#include "api/sk_config.h"

typedef enum sk_driver_type_t {
    SK_DRIVER_IMMEDIATELY = 0,
    SK_DRIVER_STDIN,
    SK_DRIVER_TCP,
    SK_DRIVER_UDP
} sk_driver_type_t;

struct sk_driver_t;
typedef struct sk_driver_opt_t {
    // Init the data field
    void (*create)  (struct sk_driver_t*);   // create driver
    void (*run)     (struct sk_driver_t*);   // start the driver
    void (*destroy) (struct sk_driver_t*);   // destroy the driver data field
} sk_driver_opt_t;

typedef struct sk_driver_t {
    sk_driver_type_t type;

    int _padding;

    sk_engine_t*   engine;
    sk_workflow_t* workflow;
    void* data;

    sk_driver_opt_t opt;
} sk_driver_t;

sk_driver_t* sk_driver_create(sk_engine_t* engine, sk_workflow_t* workflow);

void sk_driver_destroy(sk_driver_t*);

void sk_driver_run(sk_driver_t*);

#endif

