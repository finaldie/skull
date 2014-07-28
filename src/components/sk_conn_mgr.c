#include <stdlib.h>

#include "fhash/fhash.h"
#include "api/sk_conn_mgr.h"

struct sk_conn_t {
    int   fd;
};

sk_conn_t* sk_conn_create(int fd, void* ud)
{
    sk_conn_t* conn = malloc(sizeof(*conn));
    conn->fd = fd;

    return conn;
}

void sk_conn_destroy(sk_conn_t* conn)
{
    free(conn);
}

struct sk_conn_mgr_t {
    fhash* conn_mgr;
};

sk_conn_mgr_t* sk_conn_mgr_create(size_t size)
{
    sk_conn_mgr_t* mgr = malloc(sizeof(*mgr));
    mgr->conn_mgr = fhash_int_create(size, FHASH_MASK_AUTO_REHASH);
    return mgr;
}

void sk_conn_mgr_destroy(sk_conn_mgr_t* mgr)
{
    fhash_int_delete(mgr->conn_mgr);
    free(mgr);
}

void sk_conn_mgr_add(sk_conn_mgr_t* mgr, sk_conn_t* connection)
{
    fhash_int_set(mgr->conn_mgr, connection->fd, connection);
}

sk_conn_t* sk_conn_mgr_del(sk_conn_mgr_t* mgr, sk_conn_t* connection)
{
    return fhash_int_del(mgr->conn_mgr, connection->fd);
}

void sk_conn_mgr_foreach(sk_conn_mgr_t* mgr, sk_conn_each_cb each_cb, void* ud)
{
    fhash_int_iter iter = fhash_int_iter_new(mgr->conn_mgr);

    void* data = NULL;
    while ((data = fhash_int_next(&iter))) {
        if (each_cb(mgr, iter.value, ud)) {
            break;
        }
    }

    fhash_int_iter_release(&iter);
}
