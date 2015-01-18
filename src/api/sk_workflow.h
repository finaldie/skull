#ifndef SK_WORKFLOW_H
#define SK_WORKFLOW_H

#include <netinet/in.h>

#include "flibs/flist.h"
#include "api/sk_module.h"

typedef struct sk_workflow_t {
    flist* modules; // sk_module_t list
    int concurrent;

#if __WORDSIZE == 64
    int padding;
#endif
} sk_workflow_t;

sk_workflow_t* sk_workflow_create(int concurrent);
void sk_workflow_destroy(sk_workflow_t* workflow);

// @return 0 if success or non-zero if failure
int sk_workflow_add_module(sk_workflow_t* workflow, sk_module_t* module);
sk_module_t* sk_workflow_first_module(sk_workflow_t* workflow);
sk_module_t* sk_workflow_last_module(sk_workflow_t* workflow);
int sk_workflow_module_cnt(sk_workflow_t*);

#endif

