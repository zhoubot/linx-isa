/*
 * linx-libc: Minimal atomic builtins for compiler bring-up.
 *
 * These provide the GCC/Clang libatomic fallback entry points that the
 * compiler may emit when lowering C11 atomics (e.g. <stdatomic.h>).
 *
 * Notes:
 * - This is currently a non-atomic single-threaded implementation intended for
 *   compile-only/link-only tests and basic system bring-up.
 * - Memory order arguments are accepted for ABI compatibility but ignored.
 */

#include <stdint.h>

uint32_t __atomic_load_4(const volatile uint32_t *ptr, int memorder)
{
    (void)memorder;
    return *ptr;
}

void __atomic_store_4(volatile uint32_t *ptr, uint32_t val, int memorder)
{
    (void)memorder;
    *ptr = val;
}

uint32_t __atomic_exchange_4(volatile uint32_t *ptr, uint32_t val, int memorder)
{
    (void)memorder;
    uint32_t old = *ptr;
    *ptr = val;
    return old;
}

int __atomic_compare_exchange_4(
    volatile uint32_t *ptr,
    uint32_t *expected,
    uint32_t desired,
    int success_memorder,
    int failure_memorder)
{
    (void)success_memorder;
    (void)failure_memorder;

    uint32_t old = *ptr;
    if (old == *expected) {
        *ptr = desired;
        return 1;
    }
    *expected = old;
    return 0;
}

uint32_t __atomic_fetch_add_4(volatile uint32_t *ptr, uint32_t val, int memorder)
{
    (void)memorder;
    uint32_t old = *ptr;
    *ptr = old + val;
    return old;
}

uint32_t __atomic_fetch_sub_4(volatile uint32_t *ptr, uint32_t val, int memorder)
{
    (void)memorder;
    uint32_t old = *ptr;
    *ptr = old - val;
    return old;
}

uint32_t __atomic_fetch_and_4(volatile uint32_t *ptr, uint32_t val, int memorder)
{
    (void)memorder;
    uint32_t old = *ptr;
    *ptr = old & val;
    return old;
}

uint32_t __atomic_fetch_or_4(volatile uint32_t *ptr, uint32_t val, int memorder)
{
    (void)memorder;
    uint32_t old = *ptr;
    *ptr = old | val;
    return old;
}

uint32_t __atomic_fetch_xor_4(volatile uint32_t *ptr, uint32_t val, int memorder)
{
    (void)memorder;
    uint32_t old = *ptr;
    *ptr = old ^ val;
    return old;
}

void linx_sync_synchronize(void) __asm__("__sync_synchronize");
void linx_sync_synchronize(void)
{
    __asm__ __volatile__("" ::: "memory");
}
