#ifndef SK_MON_H
#define SK_MON_H

#include <stdint.h>

typedef struct sk_mon_t sk_mon_t;

sk_mon_t* sk_mon_create();
void sk_mon_destroy(sk_mon_t*);

void sk_mon_inc(sk_mon_t*, const char* name, uint32_t value);
uint32_t sk_mon_get(sk_mon_t*, const char* name);

void sk_mon_reset(sk_mon_t*);

typedef void (*sk_mon_cb)(const char* name, uint32_t value, void* ud);
void sk_mon_foreach(sk_mon_t*, sk_mon_cb cb, void* ud);

#endif

