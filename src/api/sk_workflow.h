#ifndef SK_WORKFLOW_H
#define SK_WORKFLOW_H

#include <netinet/in.h>

#include "flibs/flist.h"
#include "api/sk_types.h"
#include "api/sk_module.h"

typedef struct sk_workflow_t {
    sk_workflow_type_t type;
    int concurrent;

    union {
        // when the type == SK_WORKFLOW_TRIGGER
        struct {
            int port;
            int listen_fd;
        } network;
    } trigger;

    flist* modules; // sk_module_t list
} sk_workflow_t;

sk_workflow_t* sk_workflow_create(int concurrent, in_port_t port);
void sk_workflow_destroy(sk_workflow_t* workflow);

// @return 0 if success or non-zero if failure
int sk_workflow_add_module(sk_workflow_t* workflow, sk_module_t* module);
sk_module_t* sk_workflow_first_module(sk_workflow_t* workflow);
sk_module_t* sk_workflow_last_module(sk_workflow_t* workflow);

#endif

