#ifndef SK_ADMIN_H
#define SK_ADMIN_H

#include "api/sk_config.h"
#include "api/sk_workflow.h"
#include "api/sk_module.h"

sk_workflow_cfg_t* sk_admin_workflowcfg_create(int port);
void               sk_admin_workflowcfg_destroy(sk_workflow_cfg_t*);

sk_module_t* sk_admin_module();

#endif

