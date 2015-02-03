#ifndef SK_WORKFLOW_H
#define SK_WORKFLOW_H

#include <netinet/in.h>

#include "flibs/flist.h"
#include "api/sk_module.h"
#include "api/sk_config.h"

typedef struct sk_workflow_t {
    const sk_workflow_cfg_t* cfg; // config ref
    flist* modules;               // sk_module_t list
} sk_workflow_t;

sk_workflow_t* sk_workflow_create(const sk_workflow_cfg_t* cfg);
void sk_workflow_destroy(sk_workflow_t* workflow);

// @return 0 if success or non-zero if failure
int sk_workflow_add_module(sk_workflow_t* workflow, sk_module_t* module);
sk_module_t* sk_workflow_first_module(sk_workflow_t* workflow);
sk_module_t* sk_workflow_last_module(sk_workflow_t* workflow);
int sk_workflow_module_cnt(sk_workflow_t*);

#endif

