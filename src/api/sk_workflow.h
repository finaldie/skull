#ifndef SK_WORKFLOW_H
#define SK_WORKFLOW_H

#include "flist/flist.h"
#include "api/sk_module.h"

#define SK_WORKFLOW_NONCONCURRENT 0
#define SK_WORKFLOW_CONCURRENT    1

// the type of workflow
#define SK_WORKFLOW_MAIN    0
#define SK_WORKFLOW_NETWORK 1

typedef struct sk_workflow_t {
    int type;
    int concurrent;

    union {
        // when the type == SK_WORKFLOW_NETWORK
        struct {
            int port;
            int listen_fd;
        } network;
    } trigger;

    flist* modules; // sk_module_t list
} sk_workflow_t;

#endif

