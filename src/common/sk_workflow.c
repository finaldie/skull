#include <stdlib.h>

#include "fnet/fnet_core.h"
#include "api/sk_utils.h"
#include "api/sk_loader.h"
#include "api/sk_workflow.h"

static
sk_workflow_t* _workflow_create()
{
    sk_workflow_t* workflow = calloc(1, sizeof(*workflow));
    workflow->modules = flist_create();

    return workflow;
}

sk_workflow_t* sk_workflow_create(int concurrent, int port)
{
    sk_workflow_t* workflow = _workflow_create();
    workflow->concurrent = concurrent;

    if (port) {
        workflow->type = SK_WORKFLOW_TRIGGER;
        workflow->trigger.network.port = port;
        workflow->trigger.network.listen_fd =
            fnet_create_listen(NULL, port, 1024, 0);
    } else {
        workflow->type = SK_WORKFLOW_MAIN;
    }

    return workflow;
}

void sk_workflow_destroy(sk_workflow_t* workflow)
{
    flist_delete(workflow->modules);
    free(workflow);
}

int sk_workflow_add_module(sk_workflow_t* workflow, sk_module_t* module)
{
    return flist_push(workflow->modules, module);
}

sk_module_t* sk_workflow_first_module(sk_workflow_t* workflow)
{
    return flist_head(workflow->modules);
}

sk_module_t* sk_workflow_last_module(sk_workflow_t* workflow)
{
    return flist_tail(workflow->modules);
}
