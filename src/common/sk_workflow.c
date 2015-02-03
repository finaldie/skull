#include <stdlib.h>

#include "api/sk_utils.h"
#include "api/sk_workflow.h"

static
sk_workflow_t* _workflow_create()
{
    sk_workflow_t* workflow = calloc(1, sizeof(*workflow));
    workflow->modules = flist_create();

    return workflow;
}

sk_workflow_t* sk_workflow_create(const sk_workflow_cfg_t* cfg)
{
    sk_workflow_t* workflow = _workflow_create();
    workflow->cfg = cfg;

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

int sk_workflow_module_cnt(sk_workflow_t* workflow)
{
    if (flist_empty(workflow->modules)) {
        return 0;
    }

    flist_iter iter = flist_new_iter(workflow->modules);
    sk_module_t* module = NULL;
    int module_cnt = 0;
    while ((module = flist_each(&iter))) {
        module_cnt++;
    }

    return module_cnt;
}
