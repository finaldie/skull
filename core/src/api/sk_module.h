#ifndef SK_MODULE_H
#define SK_MODULE_H

typedef enum sk_mtype_t {
    SK_C_MODULE_TYPE = 0
} sk_mtype_t;

typedef struct sk_module_t {
    sk_mtype_t type;
    void*      md;

    int (*sk_module_init)();
    int (*sk_module_run)();
} sk_module_t;

#endif
