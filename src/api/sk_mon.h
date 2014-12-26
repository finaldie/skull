#ifndef SK_MON_H
#define SK_MON_H

typedef struct sk_mon_t sk_mon_t;

sk_mon_t* sk_mon_create();
void sk_mon_destroy(sk_mon_t*);

void sk_mon_inc(sk_mon_t*, const char* name, double value);
double sk_mon_get(sk_mon_t*, const char* name);

void sk_mon_reset(sk_mon_t*);

typedef void (*sk_mon_cb)(const char* name, double value, void* ud);
void sk_mon_foreach(sk_mon_t*, sk_mon_cb cb, void* ud);

#endif

