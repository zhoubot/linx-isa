/*
 * v0.3 Vector/Tile Block-Start Smoke Tests (strict profile)
 *
 * Bring-up goal:
 * - Ensure typed block-start markers exist as executable encodings in the toolchain
 *   and are accepted by the emulator front-end.
 *
 * NOTE:
 * This suite includes a minimal SIMT/vector body replay smoke test (v.add + v.sw.brg)
 * to validate the v0.3 bring-up execution model for MSEQ blocks.
 */

#include "linx_test.h"

/*
 * Out-of-line SIMT body for BSTART.MSEQ/MPAR tests.
 *
 * The body is executed once per LC tuple (lc0/lc1/...) and must terminate at
 * (C.)BSTOP so the emulator can replay it for the next lane.
 */
__asm__(
    ".p2align 3\n"
    ".globl __linx_v03_simt_body\n"
    "__linx_v03_simt_body:\n"
    "  v.add lc0.sw, lc1.sw, ->vt.w\n"
    "  v.sw.brg vt#1, [ri0, lc0<<2, lc1<<10]\n"
    "  C.BSTOP\n");

__asm__(
    ".p2align 3\n"
    ".globl __linx_v03_simt_copy_body\n"
    "__linx_v03_simt_copy_body:\n"
    "  v.lw.brg [ri0, lc0<<2, lc1<<10], ->vt.w\n"
    "  v.sw.brg vt#1, [ri1, lc0<<2, lc1<<10]\n"
    "  C.BSTOP\n");

__asm__(
    ".p2align 3\n"
    ".globl __linx_v03_simt_tile_body\n"
    "__linx_v03_simt_tile_body:\n"
    "  v.add lc0.sw, lc1.sw, ->vt.w\n"
    "  v.sw.local vt#1, [to, lc0<<2, lc1<<6]\n"
    "  C.BSTOP\n");

__asm__(
    ".p2align 3\n"
    ".globl __linx_v03_simt_f32_body\n"
    "__linx_v03_simt_f32_body:\n"
    "  v.lw.brg [ri0, lc0<<2, zero], ->vt\n"
    "  v.fadd vt#1, ri2, ->vt\n"
    "  v.fmul vt#1, ri3, ->vt\n"
    "  v.sw.brg vt#1, [ri1, lc0<<2, zero]\n"
    "  C.BSTOP\n");

/* Empty decoupled body used by typed block-start smoke tests. */
__asm__(
    ".p2align 2\n"
    ".globl __linx_v03_empty_body\n"
    "__linx_v03_empty_body:\n"
    "  C.BSTOP\n");

static void test_typed_block_starts_smoke(void)
{
    /*
     * Each BSTART.<type> terminates the current block and begins the next block.
     * We close each empty typed block by starting a new fall-through STD block
     * using C.BSTART. This ensures subsequent C code is still within a block.
     */
    __asm__ volatile(
        "BSTART.MSEQ 0\n"
        "B.TEXT __linx_v03_empty_body\n"
        "C.BSTART\n"
        "BSTART.MPAR 0\n"
        "B.TEXT __linx_v03_empty_body\n"
        "C.BSTART\n"
        "BSTART.VPAR 0\n"
        "B.TEXT __linx_v03_empty_body\n"
        "C.BSTART\n"
        "BSTART.VSEQ 0\n"
        "B.TEXT __linx_v03_empty_body\n"
        "C.BSTART\n"
        :
        :
        : "memory");
}

static void test_mseq_simt_store(void)
{
    enum {
        STRIDE_INTS = 256, /* 256 * 4B = 1024B stride => lc1<<10 */
        M = 64,
        N = 32,
    };

    static uint32_t a[N][STRIDE_INTS];
    for (unsigned i = 0; i < N; i++) {
        for (unsigned j = 0; j < STRIDE_INTS; j++) {
            a[i][j] = 0xDEADBEEFu;
        }
    }

    const uint64_t base = (uint64_t)(uintptr_t)&a[0][0];
    __asm__ volatile(
        "BSTART.MSEQ 0\n"
        "B.TEXT __linx_v03_simt_body\n"
        "B.IOR [%0],[]\n"
        "C.B.DIMI 64, ->lb0\n"
        "C.B.DIMI 32, ->lb1\n"
        "C.BSTART\n"
        :
        : "r"(base)
        : "memory");

    for (unsigned i = 0; i < N; i++) {
        for (unsigned j = 0; j < M; j++) {
            TEST_EQ32(a[i][j], (uint32_t)(i + j), 0x1201);
        }
        for (unsigned j = M; j < STRIDE_INTS; j++) {
            TEST_EQ32(a[i][j], 0xDEADBEEFu, 0x1202);
        }
    }
}

static void test_mseq_simt_copy(void)
{
    enum {
        STRIDE_INTS = 256, /* 256 * 4B = 1024B stride => lc1<<10 */
        M = 64,
        N = 8,
    };

    static uint32_t src[N][STRIDE_INTS];
    static uint32_t dst[N][STRIDE_INTS];
    for (unsigned i = 0; i < N; i++) {
        for (unsigned j = 0; j < STRIDE_INTS; j++) {
            src[i][j] = 0x11100000u + (i << 12) + j;
            dst[i][j] = 0;
        }
    }

    const uint64_t src_base = (uint64_t)(uintptr_t)&src[0][0];
    const uint64_t dst_base = (uint64_t)(uintptr_t)&dst[0][0];
    __asm__ volatile(
        "BSTART.MSEQ 0\n"
        "B.TEXT __linx_v03_simt_copy_body\n"
        "B.IOR [%0],[]\n" /* ri0 */
        "B.IOR [%1],[]\n" /* ri1 */
        "C.B.DIMI 64, ->lb0\n"
        "C.B.DIMI 8, ->lb1\n"
        "C.BSTART\n"
        :
        : "r"(src_base), "r"(dst_base)
        : "memory");

    for (unsigned i = 0; i < N; i++) {
        for (unsigned j = 0; j < M; j++) {
            TEST_EQ32(dst[i][j], src[i][j], 0x1210);
        }
        for (unsigned j = M; j < STRIDE_INTS; j++) {
            TEST_EQ32(dst[i][j], 0u, 0x1211);
        }
    }
}

static void test_vseq_local_tile_store(void)
{
    enum {
        M = 16,
        N = 16,
        TILE_WORDS = 4096 / 4,
    };

    static uint32_t out[TILE_WORDS];
    for (unsigned i = 0; i < TILE_WORDS; i++) {
        out[i] = 0xDEADBEEFu;
    }

    __asm__ volatile(
        "BSTART.VSEQ 0\n"
        "B.TEXT __linx_v03_simt_tile_body\n"
        "B.IOTI [], last ->t<4KB>\n"
        "C.B.DIMI 16, ->lb0\n"
        "C.B.DIMI 16, ->lb1\n"
        "C.BSTART\n"
        :
        :
        : "memory");

    const uint64_t out_base = (uint64_t)(uintptr_t)&out[0];
    __asm__ volatile(
        "BSTART.TMA 0, 1\n" /* dtype=INT32(0), func=TSTORE(1) */
        "B.ARG NORM.normal\n"
        "B.IOR [%0],[]\n"
        "B.IOTI [t#1], last ->t<4KB>\n"
        "C.BSTART\n"
        :
        : "r"(out_base)
        : "memory");

    for (unsigned i = 0; i < N; i++) {
        for (unsigned j = 0; j < M; j++) {
            TEST_EQ32(out[i * M + j], (uint32_t)(i + j), 0x1220);
        }
    }
    for (unsigned i = N * M; i < TILE_WORDS; i++) {
        TEST_EQ32(out[i], 0u, 0x1221);
    }
}

static void test_mseq_simt_f32_smoke(void)
{
    enum { N = 64 };

    static float src[N];
    static float dst[N];

    for (unsigned i = 0; i < N; i++) {
        src[i] = (float)i;
        dst[i] = 0.0f;
    }

    const uint64_t src_base = (uint64_t)(uintptr_t)&src[0];
    const uint64_t dst_base = (uint64_t)(uintptr_t)&dst[0];
    const uint64_t add1_f32 = 0x3f800000u; /* 1.0f */
    const uint64_t mul2_f32 = 0x40000000u; /* 2.0f */

    __asm__ volatile(
        "BSTART.MSEQ 0\n"
        "B.TEXT __linx_v03_simt_f32_body\n"
        "B.IOR [%0, %1, %2],[]\n"
        "B.IOR [%3],[]\n"
        "C.B.DIMI 64, ->lb0\n"
        "C.BSTART\n"
        :
        : "r"(src_base), "r"(dst_base), "r"(add1_f32), "r"(mul2_f32)
        : "memory");

    for (unsigned i = 0; i < N; i++) {
        union {
            float f;
            uint32_t u;
        } a, e;
        a.f = dst[i];
        e.f = ((float)i + 1.0f) * 2.0f;
        TEST_EQ32(a.u, e.u, 0x1230u + i);
    }
}

void run_v03_vector_tile_tests(void)
{
    test_start(0x1200);
    uart_puts("v0.3 typed BSTART.* smoke ... ");

    test_typed_block_starts_smoke();

    test_pass();

    test_start(0x1201);
    uart_puts("v0.3 MSEQ SIMT store ... ");

    test_mseq_simt_store();

    test_pass();

    test_start(0x1210);
    uart_puts("v0.3 MSEQ SIMT copy ... ");

    test_mseq_simt_copy();

    test_pass();

    test_start(0x1220);
    uart_puts("v0.3 VSEQ local tile store ... ");

    test_vseq_local_tile_store();

    test_pass();

    test_start(0x1230);
    uart_puts("v0.3 MSEQ SIMT f32 smoke ... ");

    test_mseq_simt_f32_smoke();

    test_pass();
}
