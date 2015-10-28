#include <stdlib.h>

#include "api/sk_object.h"

struct sk_obj_t {
    sk_obj_opt_t opt;
    sk_ud_t      ud;
};

sk_obj_t* sk_obj_create(sk_obj_opt_t opt)
{
    sk_obj_t* obj = calloc(1, sizeof(*obj));
    obj->opt = opt;

    return obj;
}

void      sk_obj_destroy(sk_obj_t* obj)
{
    if (!obj) return;

    if (obj->opt.destroy) {
        obj->opt.destroy(obj->ud);
    }

    free(obj);
}

sk_ud_t   sk_obj_get(sk_obj_t* obj)
{
    return obj->ud;
}

void      sk_obj_set(sk_obj_t* obj, sk_ud_t ud)
{
    if (obj->opt.preset) {
        obj->opt.preset(obj->ud);
    }

    obj->ud = ud;
}
