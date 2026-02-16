#include "linx_test.h"

#include <stdint.h>

typedef uint64_t (*u64_fn)(uint64_t);

static __attribute__((noinline)) uint64_t inc7(uint64_t x) { return x + 7; }
static __attribute__((noinline)) uint64_t dec3(uint64_t x) { return x - 3; }
static __attribute__((noinline)) uint64_t mul2(uint64_t x) { return x * 2; }
static u64_fn g_dispatch_table[3] = {inc7, dec3, mul2};

static __attribute__((noinline)) uint64_t direct_chain(uint64_t x) {
    return dec3(inc7(x));
}

static __attribute__((noinline)) uint64_t nested_chain(uint64_t x) {
    uint64_t a = direct_chain(x + 1);
    uint64_t b = mul2(a + 2);
    return direct_chain(b);
}

static __attribute__((noinline)) uint64_t recursive_sum(uint64_t n) {
    if (n == 0)
        return 0;
    return n + recursive_sum(n - 1);
}

static __attribute__((noinline)) uint64_t mutual_even(uint64_t n);
static __attribute__((noinline)) uint64_t mutual_odd(uint64_t n);

static __attribute__((noinline)) uint64_t mutual_even(uint64_t n) {
    if (n == 0)
        return 1;
    return mutual_odd(n - 1);
}

static __attribute__((noinline)) uint64_t mutual_odd(uint64_t n) {
    if (n == 0)
        return 0;
    return mutual_even(n - 1);
}

static __attribute__((noinline)) uint64_t indirect_dispatch(u64_fn fn, uint64_t x) {
    return fn(x);
}

static __attribute__((noinline)) uint64_t indirect_dispatch_table(uint64_t idx, uint64_t x) {
    return g_dispatch_table[idx](x);
}

static __attribute__((noinline)) uint64_t tail_target(uint64_t x) { return x + 11; }
static volatile u64_fn g_tail_fn = tail_target;

static __attribute__((noinline)) uint64_t tail_direct(uint64_t x) {
    __attribute__((musttail)) return tail_target(x);
}

static __attribute__((noinline)) uint64_t tail_indirect(uint64_t x) {
    u64_fn fn = g_tail_fn;
    __attribute__((musttail)) return fn(x);
}

static __attribute__((noinline)) uint64_t frame_heavy_leaf(uint64_t x) { return x + 5; }

extern uint64_t callret_tpl_fret_stk_slot_redirect(uint64_t x);
extern uint64_t callret_tpl_fret_ra_slot_redirect(uint64_t x);

static __attribute__((noinline)) uint64_t frame_heavy(uint64_t x) {
    volatile uint64_t s0 = x + 1;
    volatile uint64_t s1 = x + 2;
    volatile uint64_t s2 = x + 3;
    volatile uint64_t s3 = x + 4;
    volatile uint64_t s4 = x + 5;
    volatile uint64_t s5 = x + 6;
    volatile uint64_t s6 = x + 7;
    uint64_t slots[8];
    slots[0] = s0;
    slots[1] = s1;
    slots[2] = s2;
    slots[3] = s3;
    slots[4] = s4;
    slots[5] = s5;
    slots[6] = s6;
    slots[7] = s6 + x;
    uint64_t y = frame_heavy_leaf(slots[0] + slots[7]);
    return y + slots[3] + slots[5];
}

static void test_direct_calls(void) {
    uint64_t r = direct_chain(10);
    TEST_EQ64(r, 14, 0x1401);
}

static void test_nested_calls(void) {
    uint64_t r = nested_chain(4);
    TEST_EQ64(r, 26, 0x1402);
}

static void test_recursive_calls(void) {
    uint64_t r = recursive_sum(8);
    TEST_EQ64(r, 36, 0x1403);
}

static void test_indirect_calls(void) {
    uint64_t a = indirect_dispatch(inc7, 3);
    uint64_t b = indirect_dispatch(dec3, a);
    TEST_EQ64(b, 7, 0x1404);
}

static void test_indirect_table_calls(void) {
    uint64_t a = indirect_dispatch_table(0, 8);
    uint64_t b = indirect_dispatch_table(1, a);
    uint64_t c = indirect_dispatch_table(2, b);
    TEST_EQ64(c, 24, 0x1407);
}

static void test_mutual_recursive_calls(void) {
    uint64_t e = mutual_even(12);
    uint64_t o = mutual_odd(12);
    TEST_EQ64(e, 1, 0x1408);
    TEST_EQ64(o, 0, 0x1408);
}

static void test_tail_direct(void) {
    uint64_t r = tail_direct(19);
    TEST_EQ64(r, 30, 0x1405);
}

static void test_tail_indirect(void) {
    uint64_t r = tail_indirect(2);
    TEST_EQ64(r, 13, 0x1406);
}

static void test_tail_indirect_rebind(void) {
    g_tail_fn = inc7;
    uint64_t r = tail_indirect(2);
    TEST_EQ64(r, 9, 0x1409);
    g_tail_fn = tail_target;
}

static void test_frame_heavy_return(void) {
    uint64_t r = frame_heavy(10);
    TEST_EQ64(r, 73, 0x140a);
}

static void test_fret_stk_uses_stack_ra(void) {
    uint64_t r = callret_tpl_fret_stk_slot_redirect(0);
    TEST_EQ64(r, 0x22, 0x140b);
}

static void test_fret_ra_uses_snapshot_ra(void) {
    uint64_t r = callret_tpl_fret_ra_slot_redirect(0);
    TEST_EQ64(r, 0x33, 0x140c);
}

void run_callret_tests(void) {
    test_suite_begin(0x1400);
    RUN_TEST(test_direct_calls, 0x1401);
    RUN_TEST(test_nested_calls, 0x1402);
    RUN_TEST(test_recursive_calls, 0x1403);
    RUN_TEST(test_indirect_calls, 0x1404);
    RUN_TEST(test_tail_direct, 0x1405);
    RUN_TEST(test_tail_indirect, 0x1406);
    RUN_TEST(test_indirect_table_calls, 0x1407);
    RUN_TEST(test_mutual_recursive_calls, 0x1408);
    RUN_TEST(test_tail_indirect_rebind, 0x1409);
    RUN_TEST(test_frame_heavy_return, 0x140a);
    RUN_TEST(test_fret_stk_uses_stack_ra, 0x140b);
    RUN_TEST(test_fret_ra_uses_snapshot_ra, 0x140c);
    test_suite_end(12, 12);
}
