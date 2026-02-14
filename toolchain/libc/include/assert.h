#ifndef _LINX_ASSERT_H
#define _LINX_ASSERT_H

#include <linxisa_libc.h>

#ifdef __cplusplus
extern "C" {
#endif

__attribute__((noreturn)) static inline void __linx_assert_fail(void)
{
    abort();
}

#ifdef __cplusplus
}
#endif

#ifndef NDEBUG
#define assert(expr) ((expr) ? (void)0 : __linx_assert_fail())
#else
#define assert(expr) ((void)0)
#endif

#endif /* _LINX_ASSERT_H */
