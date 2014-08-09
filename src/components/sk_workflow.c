#include <stdlib.h>

#include "flist/flist.h"
#include "api/sk_utils.h"
#include "api/sk_module.h"
#include "api/sk_workflow.h"

sk_workflow_t* sk_workflow_create()
{
    sk_workflow_t* workflow = calloc(1, sizeof(*workflow));
    workflow->modules = flist_create();
    return workflow;
}

void sk_workflow_destroy(sk_workflow_t* workflow)
{
    flist_delete(workflow->modules);
    free(workflow);
}
