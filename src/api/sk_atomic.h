#ifndef SK_ATOMIC_H
#define SK_ATOMIC_H

/**
 * Inner atomic operations. TODO: Use C11 __Atomic to replace all these
 */
#define SK_ATOMIC_INC(v) __sync_add_and_fetch(&(v), 1)
#define SK_ATOMIC_ADD(v, delta) __sync_add_and_fetch(&(v), (delta))
#define SK_ATOMIC_CAS(v, expect, value) \
    __sync_bool_compare_and_swap(&(v), (expect), (value))

#endif

