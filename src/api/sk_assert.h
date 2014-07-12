#ifndef SK_ASSERT_H
#define SK_ASSERT_H

#define SK_ASSERT(expr) \
    if (!(expr)) { \
        sk_assert_exit(#expr, __FILE__, __LINE__); \
    }

void sk_assert_exit(const char* expr, const char* file, int lineno);

#endif

