#ifndef SK_OBJECT_H
#define SK_OBJECT_H

#include "api/sk_types.h"

// All the methods are optional, if user do not define the behavior, the defaut
//  behavior will apply:
//  1) For preset: just replace the internal sk_ud_t
//  2) For destroy: do nothing
typedef struct sk_obj_opt_t {
    void (*preset)  (sk_ud_t old_ud);
    void (*destroy) (sk_ud_t ud);
} sk_obj_opt_t;

typedef struct sk_obj_t sk_obj_t;

sk_obj_t* sk_obj_create  (sk_obj_opt_t opt, sk_ud_t ud);
void      sk_obj_destroy (sk_obj_t*);
sk_ud_t   sk_obj_get     (sk_obj_t*);
void      sk_obj_set     (sk_obj_t*, sk_ud_t ud);

#endif

