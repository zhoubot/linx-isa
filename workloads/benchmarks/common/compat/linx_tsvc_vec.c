#include <stddef.h>
#include <stdint.h>
/*
 * TSVC vector smoke path (strict v0.3 bring-up).
 *
 * This provides a small decoupled vector block that exercises:
 * - BSTART.MSEQ/BSTART.MPAR + B.TEXT + B.IOR + C.B.DIMI header descriptors
 * - v.lw.brg / v.sw.brg global accesses via ri*
 * - v.fadd / v.fmul floating-point vector ops (fs32 on bring-up)
 *
 * It is intentionally independent of TSVC arrays so checksums remain stable.
 */
__attribute__((aligned(64))) static float linx_tsvc_vec_in[64];
__attribute__((aligned(64))) static float linx_tsvc_vec_out[64];

#ifndef LINX_TSVC_VECTOR_MODE
#define LINX_TSVC_VECTOR_MODE 0
#endif

#if LINX_TSVC_VECTOR_MODE == 0
#define LINX_TSVC_VBLOCK_BSTART "BSTART.MSEQ 0\n"
#elif LINX_TSVC_VECTOR_MODE == 1
#define LINX_TSVC_VBLOCK_BSTART "BSTART.MPAR 0\n"
#else
#error "unsupported LINX_TSVC_VECTOR_MODE (expected 0=mseq, 1=mpar)"
#endif

__asm__(
    ".p2align 3\n"
    ".globl __linx_tsvc_vec_body\n"
    "__linx_tsvc_vec_body:\n"
    "  v.lw.brg [ri0, lc0<<2, zero], ->vt\n"
    "  v.fadd vt#1, ri2, ->vt\n"
    "  v.fmul vt#1, ri3, ->vt\n"
    "  v.sw.brg vt#1, [ri1, lc0<<2, zero]\n"
    "  C.BSTOP\n");

void linx_tsvc_vec_smoke(void)
{
    const uint64_t in_base = (uint64_t)(uintptr_t)&linx_tsvc_vec_in[0];
    const uint64_t out_base = (uint64_t)(uintptr_t)&linx_tsvc_vec_out[0];
    const uint64_t add1_f32 = 0x3f800000u; /* 1.0f */
    const uint64_t mul2_f32 = 0x40000000u; /* 2.0f */

    __asm__ volatile(
        LINX_TSVC_VBLOCK_BSTART
        "B.TEXT __linx_tsvc_vec_body\n"
        "B.IOR [%0, %1, %2],[]\n" /* ri0=in_base, ri1=out_base, ri2=add1_f32 */
        "B.IOR [%3],[]\n"         /* ri3=mul2_f32 */
        "C.B.DIMI 64, ->lb0\n"
        "C.BSTART\n"
        :
        : "r"(in_base), "r"(out_base), "r"(add1_f32), "r"(mul2_f32)
        : "memory");
}
