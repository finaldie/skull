#ifndef SK_MON_H
#define SK_MON_H

#include <time.h>

struct sk_core_t;

typedef struct sk_mon_t sk_mon_t;
typedef struct sk_mon_snapshot_t sk_mon_snapshot_t;

/*********************************sk mon APIs**********************************/
sk_mon_t* sk_mon_create();
void sk_mon_destroy(sk_mon_t*);

void sk_mon_inc(sk_mon_t*, const char* name, double value);
void sk_mon_set(sk_mon_t*, const char* name, double value);
double sk_mon_get(sk_mon_t*, const char* name);

/**
 * @param loc   0: current, -1: previous one (curr - 1), -2: current - 2..
 */
sk_mon_snapshot_t* sk_mon_snapshot(sk_mon_t*, int loc);
void sk_mon_reset_and_snapshot(sk_mon_t*);

void sk_mon_snapshot_all(struct sk_core_t* core);

/*******************************Snapshot APIs**********************************/
void sk_mon_snapshot_destroy(sk_mon_snapshot_t*);
time_t sk_mon_snapshot_starttime(sk_mon_snapshot_t*);
time_t sk_mon_snapshot_endtime(sk_mon_snapshot_t*);

typedef void (*sk_mon_cb)(const char* name, double value, void* ud);
void sk_mon_snapshot_foreach(sk_mon_snapshot_t*, sk_mon_cb cb, void* ud);

#endif

