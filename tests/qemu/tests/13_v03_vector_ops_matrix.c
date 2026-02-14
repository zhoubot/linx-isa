/*
 * v0.3 Vector operation matrix tests (MSEQ body execution path).
 *
 * Coverage intent:
 * - Integer vector ALU + bridged path: v.add / v.sub + v.lw.brg / v.sw.brg
 * - Floating-point vector ALU: v.fadd / v.fmul
 */

#include "linx_test.h"

__asm__(
    ".p2align 3\n"
    ".globl __linx_v03_ops_add_sub_body\n"
    "__linx_v03_ops_add_sub_body:\n"
    "  v.lw.brg [ri0, lc0<<2, zero], ->vt\n"
    "  v.lw.brg [ri1, lc0<<2, zero], ->vu\n"
    "  v.add vt#1, vu#1, ->vm\n"
    "  v.sub vt#1, vu#1, ->vn\n"
    "  v.sw.brg vm#1, [ri2, lc0<<2, zero]\n"
    "  v.sw.brg vn#1, [ri3, lc0<<2, zero]\n"
    "  C.BSTOP\n");

__asm__(
    ".p2align 3\n"
    ".globl __linx_v03_ops_float_body\n"
    "__linx_v03_ops_float_body:\n"
    "  v.lw.brg [ri0, lc0<<2, zero], ->vt\n"
    "  v.fadd vt#1, ri2, ->vt\n"
    "  v.fmul vt#1, ri3, ->vt\n"
    "  v.sw.brg vt#1, [ri1, lc0<<2, zero]\n"
    "  C.BSTOP\n");

__asm__(
    ".p2align 3\n"
    ".globl __linx_v03_ops_mixed_pred_body\n"
    "__linx_v03_ops_mixed_pred_body:\n"
    "  addi a7, 1, ->a7\n"
    "  v.cmp.lt lc0.sw, ri1.sw, ->vm\n"
    "  v.sw.brg vm#1, [ri0, lc0<<2, zero]\n"
    "  C.BSTOP\n");

static void test_v_add_sub_matrix(void)
{
    enum { N = 32 };

    static uint32_t a[N];
    static uint32_t b[N];
    static uint32_t sum[N];
    static uint32_t diff[N];

    for (unsigned i = 0; i < N; i++) {
        a[i] = 100u + i * 3u;
        b[i] = 17u + i;
        sum[i] = 0u;
        diff[i] = 0u;
    }

    const uint64_t a_base = (uint64_t)(uintptr_t)&a[0];
    const uint64_t b_base = (uint64_t)(uintptr_t)&b[0];
    const uint64_t sum_base = (uint64_t)(uintptr_t)&sum[0];
    const uint64_t diff_base = (uint64_t)(uintptr_t)&diff[0];

    __asm__ volatile(
        "BSTART.MSEQ 0\n"
        "B.TEXT __linx_v03_ops_add_sub_body\n"
        "B.IOR [%0, %1, %2],[]\n"
        "B.IOR [%3],[]\n"
        "C.B.DIMI 32, ->lb0\n"
        "C.BSTART\n"
        :
        : "r"(a_base), "r"(b_base), "r"(sum_base), "r"(diff_base)
        : "memory");

    for (unsigned i = 0; i < N; i++) {
        TEST_EQ32(sum[i], a[i] + b[i], 0x1301u + i);
        TEST_EQ32(diff[i], a[i] - b[i], 0x1321u + i);
    }
}

static void test_v_float_matrix(void)
{
    enum { N = 32 };

    static float src[N];
    static float dst[N];

    for (unsigned i = 0; i < N; i++) {
        src[i] = (float)i * 0.25f;
        dst[i] = 0.0f;
    }

    const uint64_t src_base = (uint64_t)(uintptr_t)&src[0];
    const uint64_t dst_base = (uint64_t)(uintptr_t)&dst[0];
    const uint64_t add_f32 = 0x3f800000u; /* +1.0f */
    const uint64_t mul_f32 = 0x40000000u; /* *2.0f */

    __asm__ volatile(
        "BSTART.MSEQ 0\n"
        "B.TEXT __linx_v03_ops_float_body\n"
        "B.IOR [%0, %1, %2],[]\n"
        "B.IOR [%3],[]\n"
        "C.B.DIMI 32, ->lb0\n"
        "C.BSTART\n"
        :
        : "r"(src_base), "r"(dst_base), "r"(add_f32), "r"(mul_f32)
        : "memory");

    for (unsigned i = 0; i < N; i++) {
        union {
            float f;
            uint32_t u;
        } actual, expect;
        actual.f = dst[i];
        expect.f = (src[i] + 1.0f) * 2.0f;
        TEST_EQ32(actual.u, expect.u, 0x1340u + i);
    }
}

static void test_v_mixed_scalar_vector_predicate(void)
{
    enum { N = 32 };

    static uint32_t out[N];
    for (unsigned i = 0; i < N; i++) {
        out[i] = 0u;
    }

    const uint64_t out_base = (uint64_t)(uintptr_t)&out[0];
    const uint64_t threshold = 12u;
    uint64_t lane_counter = 0u;

    __asm__ volatile(
        "addi zero, 0, ->a7\n"
        "BSTART.MSEQ 0\n"
        "B.TEXT __linx_v03_ops_mixed_pred_body\n"
        "B.IOR [%1, %2],[]\n"
        "C.B.DIMI 32, ->lb0\n"
        "C.BSTART\n"
        "add a7, zero, ->%0\n"
        : "=r"(lane_counter)
        : "r"(out_base), "r"(threshold)
        : "a7", "memory");

    TEST_EQ64(lane_counter, N, 0x1360);

    for (unsigned i = 0; i < N; i++) {
        const uint32_t expect = (i < threshold) ? 1u : 0u;
        TEST_EQ32(out[i], expect, 0x1361u + i);
    }
}

void run_v03_vector_ops_matrix_tests(void)
{
    test_start(0x1300);
    uart_puts("v0.3 vector add/sub matrix ... ");
    test_v_add_sub_matrix();
    test_pass();

    test_start(0x1310);
    uart_puts("v0.3 vector float matrix ... ");
    test_v_float_matrix();
    test_pass();

    test_start(0x1320);
    uart_puts("v0.3 mixed scalar/vector predicate ... ");
    test_v_mixed_scalar_vector_predicate();
    test_pass();
}
