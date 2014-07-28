#ifndef SK_CONN_MGR_H
#define SK_CONN_MGR_H

#include <stddef.h>

typedef struct sk_conn_t sk_conn_t;

sk_conn_t* sk_conn_create(int fd, void* ud);
void sk_conn_destroy(sk_conn_t* connetion);

typedef struct sk_conn_mgr_t sk_conn_mgr_t;

sk_conn_mgr_t* sk_conn_mgr_create(size_t size);
void sk_conn_mgr_destroy(sk_conn_mgr_t* mgr);
void sk_conn_mgr_add(sk_conn_mgr_t* mgr, sk_conn_t* connection);
sk_conn_t* sk_conn_mgr_del(sk_conn_mgr_t* mgr, sk_conn_t* connection);

// return 0: continue iterating
// return non-zero: stop iterating
typedef int (*sk_conn_each_cb)(sk_conn_mgr_t* mgr,
                               sk_conn_t* connection,
                               void* ud);

void sk_conn_mgr_foreach(sk_conn_mgr_t* mgr, sk_conn_each_cb each_cb, void* ud);

#endif

