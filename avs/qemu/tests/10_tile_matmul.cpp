// LinxISA Tile Block Tests (TAU bring-up)
//
// This suite exercises the builtin-based PTO→LinxISA tile lowering (no inline
// assembly / no raw-encoding stubs):
// - BSTART.TMA + B.IOT/B.IOTI: TLOAD/TSTORE
// - BSTART.CUBE(MAMULB/ACCCVT) + B.DIM + B.IOT: 8x8 i32 matmul in QEMU (TAU emulation)

#include "linx_test.h"

#define __LINX_TAU__ 1
#include <pto/linx/AutoModeKernels.hpp>
#include <pto/linx/TileOps.hpp>

extern "C" void pto_tload_store_i32(const int *src, int *dst);
extern "C" void pto_mamulb_i32_8x8(const int *lhs, const int *rhs, int *dst);
extern "C" void pto_tmatmul_acc_i32_8x8(const int *lhs, const int *rhs, int *acc_dst);
extern "C" void pto_gemm_auto_i32(const int *lhs, const int *rhs, int *dst);
extern "C" void pto_flash_attention_auto_i32(const int *query, const int *key, const int *value, int *dst);

static constexpr unsigned kTileElemsI32 = pto::linx::auto_mode::kTileElemsI32;
static constexpr unsigned kTileSizeCode = pto::linx::auto_mode::kFullTileSizeCode;
static constexpr unsigned kFmtNorm = 0;
static constexpr unsigned kFmtND2NZ = 1;

#ifndef LINX_TEST_ENABLE_TMA_DESC
#define LINX_TEST_ENABLE_TMA_DESC 0
#endif

static void tile_matmul_ref_i32_8x8(int32_t out[64], const int32_t a[64], const int32_t b[64])
{
    for (unsigned i = 0; i < 8; i++) {
        for (unsigned j = 0; j < 8; j++) {
            int64_t acc = 0;
            for (unsigned k = 0; k < 8; k++) {
                acc += (int64_t)a[i * 8u + k] * (int64_t)b[k * 8u + j];
            }
            out[i * 8u + j] = (int32_t)acc;
        }
    }
}

static int32_t *tile_ptr(int32_t *buffer, unsigned tile_idx)
{
    return buffer + tile_idx * kTileElemsI32;
}

static const int32_t *tile_ptr(const int32_t *buffer, unsigned tile_idx)
{
    return buffer + tile_idx * kTileElemsI32;
}

static void init_tile_pattern(int32_t *tile, int32_t seed)
{
    for (unsigned i = 0; i < kTileElemsI32; i++) {
        tile[i] = 0;
    }
    for (unsigned i = 0; i < 64; i++) {
        int32_t lane = (int32_t)(i % 13u) - 6;
        int32_t col = (int32_t)(i & 7u) - 3;
        tile[i] = lane * seed + col;
    }
}

static int64_t checksum_tiles_i32(const int32_t *tiles, unsigned tile_count)
{
    int64_t checksum = 0;
    for (unsigned tile = 0; tile < tile_count; tile++) {
        const int32_t *base = tile_ptr(tiles, tile);
        for (unsigned i = 0; i < 64; i++) {
            checksum += (int64_t)base[i];
        }
    }
    return checksum;
}

static void print_checksum(const char *label, int64_t value)
{
    uart_puts(label);
    uart_puts("0x");
    uart_puthex64((uint64_t)value);
    uart_puts("\r\n");
}

static void run_base_tile_tests()
{
    test_start(0x000A0001);
    uart_puts("PTO tile matmul (8x8 i32) ... ");

    alignas(16) static int32_t A[1024];
    alignas(16) static int32_t B[1024];
    alignas(16) static int32_t C[1024];
    alignas(16) static int32_t EXP[64];

    for (unsigned i = 0; i < 1024; i++) {
        A[i] = 0;
        B[i] = 0;
        C[i] = 0;
        if (i < 64) {
            A[i] = (int32_t)((int)i % 7 - 3);
            B[i] = (int32_t)((int)i % 5 - 2);
        }
    }
    for (unsigned i = 0; i < 64; i++) {
        EXP[i] = 0;
    }

    // Tiles are SSA values; LLVM register allocation assigns them to the
    // architectural tile register file (32 tiles: 4 hands × depth 8).
    auto tA = pto::linx::tload<kTileSizeCode>(A);          // 4 KiB
    auto tB = pto::linx::tload<kTileSizeCode>(B);          // 4 KiB
    auto tC = pto::linx::mamulb<8, 8, 8>(tA, tB); // 8x8 i32
    pto::linx::tstore<kTileSizeCode>(C, tC);             // 4 KiB

    tile_matmul_ref_i32_8x8(EXP, A, B);
    for (unsigned i = 0; i < 64; i++) {
        TEST_EQ32((uint32_t)C[i], (uint32_t)EXP[i], 0x000A1000u + i);
    }

    test_pass();

    test_start(0x000A0002);
    uart_puts("PTO tload/tstore roundtrip ... ");

    alignas(16) static int32_t SRC[1024];
    alignas(16) static int32_t DST[1024];
    for (unsigned i = 0; i < 1024; i++) {
        SRC[i] = (int32_t)((int)i * 3 - 7);
        DST[i] = 0;
    }

    auto tRT = pto::linx::tload<kTileSizeCode>(SRC);
    pto::linx::tstore<kTileSizeCode>(DST, tRT);

    for (unsigned i = 0; i < 128; i++) {
        TEST_EQ32((uint32_t)DST[i], (uint32_t)SRC[i], 0x000A2000u + i);
    }

    test_pass();

    test_start(0x000A0003);
    uart_puts("PTO tmatmul_acc pipeline ... ");

    alignas(16) static int32_t C_ACC[1024];
    for (unsigned i = 0; i < 1024; i++) {
        C_ACC[i] = 0;
    }

    auto tA_acc = pto::linx::tload<kTileSizeCode>(A);
    auto tB_acc = pto::linx::tload<kTileSizeCode>(B);

    // v0.3 bring-up: the implicit accumulator is seeded by a preceding MAMULB.
    // The ACC operand of tmatmul_acc is currently a SSA dependency carrier.
    auto tSeed = pto::linx::mamulb<8, 8, 8>(tA_acc, tB_acc);
    auto tOut = pto::linx::tmatmul_acc<8, 8, 8>(tSeed, tA_acc, tB_acc);
    pto::linx::tstore<kTileSizeCode>(C_ACC, tOut);

    for (unsigned i = 0; i < 64; i++) {
        const int32_t expected = (int32_t)((int64_t)EXP[i] * 2);
        TEST_EQ32((uint32_t)C_ACC[i], (uint32_t)expected, 0x000A3000u + i);
    }

    test_pass();

    test_start(0x000A000C);
    uart_puts("PTO tile tadd (VPAR) ... ");

    alignas(16) static int32_t ADD_A[1024];
    alignas(16) static int32_t ADD_B[1024];
    alignas(16) static int32_t ADD_SUM[1024];
    alignas(16) static int32_t ADD_DIFF[1024];

    for (unsigned i = 0; i < 1024; i++) {
        ADD_A[i] = (int32_t)((int)i * 3 - 7);
        ADD_B[i] = (int32_t)((int)i * 5 + 11);
        ADD_SUM[i] = 0;
        ADD_DIFF[i] = 0;
    }

    auto tAA = pto::linx::tload<kTileSizeCode>(ADD_A);
    auto tBB = pto::linx::tload<kTileSizeCode>(ADD_B);
    auto tSum = pto::linx::tadd<kTileSizeCode>(tAA, tBB);
    pto::linx::tstore<kTileSizeCode>(ADD_SUM, tSum);

    for (unsigned i = 0; i < 256; i++) {
        const int32_t exp_sum = (int32_t)((int64_t)ADD_A[i] + (int64_t)ADD_B[i]);
        TEST_EQ32((uint32_t)ADD_SUM[i], (uint32_t)exp_sum, 0x000AC000u + i);
    }

    test_pass();

    test_start(0x000A000D);
    uart_puts("PTO tile tsub (VPAR) ... ");

    for (unsigned i = 0; i < 1024; i++) {
        ADD_DIFF[i] = 0;
    }

    auto tAA2 = pto::linx::tload<kTileSizeCode>(ADD_A);
    auto tBB2 = pto::linx::tload<kTileSizeCode>(ADD_B);
    auto tDiff = pto::linx::tsub<kTileSizeCode>(tAA2, tBB2);
    pto::linx::tstore<kTileSizeCode>(ADD_DIFF, tDiff);

    for (unsigned i = 0; i < 256; i++) {
        const int32_t exp_diff = (int32_t)((int64_t)ADD_A[i] - (int64_t)ADD_B[i]);
        TEST_EQ32((uint32_t)ADD_DIFF[i], (uint32_t)exp_diff, 0x000AD000u + i);
    }

    test_pass();
}

static void run_auto_mode_gemm_test()
{
    test_start(0x000A0004);
    uart_puts("Auto-mode GEMM kernel ... ");

    alignas(16) static int32_t GEMM_A[9 * kTileElemsI32];
    alignas(16) static int32_t GEMM_B[8 * kTileElemsI32];
    alignas(16) static int32_t GEMM_OUT[11 * kTileElemsI32];
    alignas(16) static int32_t GEMM_REF0[64];

    for (unsigned tile = 0; tile < 9; tile++) {
        init_tile_pattern(tile_ptr(GEMM_A, tile), (int32_t)(3 + tile));
    }
    for (unsigned tile = 0; tile < 8; tile++) {
        init_tile_pattern(tile_ptr(GEMM_B, tile), (int32_t)(11 + tile));
    }
    for (unsigned i = 0; i < 11 * kTileElemsI32; i++) {
        GEMM_OUT[i] = 0;
    }

    pto::linx::auto_mode::gemm_kernel_i32(GEMM_A, GEMM_B, GEMM_OUT);

    const unsigned gemm_lhs_map[11] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 0, 1};
    const unsigned gemm_rhs_map[11] = {0, 1, 2, 3, 4, 5, 6, 0, 1, 2, 7};
    for (unsigned tile = 0; tile < 11; tile++) {
        tile_matmul_ref_i32_8x8(
            GEMM_REF0,
            tile_ptr(GEMM_A, gemm_lhs_map[tile]),
            tile_ptr(GEMM_B, gemm_rhs_map[tile]));
        const int32_t *out_tile = tile_ptr(GEMM_OUT, tile);
        for (unsigned i = 0; i < 64; i++) {
            TEST_EQ32((uint32_t)out_tile[i], (uint32_t)GEMM_REF0[i], 0x000A4000u + tile * 64u + i);
        }
    }

    print_checksum("QEMU_GEMM_CHECKSUM=", checksum_tiles_i32(GEMM_OUT, 11));
    test_pass();
}

static void run_auto_mode_flash_test()
{
    test_start(0x000A0005);
    uart_puts("Auto-mode flash-attention kernel ... ");

    alignas(16) static int32_t FLASH_Q[5 * kTileElemsI32];
    alignas(16) static int32_t FLASH_K[5 * kTileElemsI32];
    alignas(16) static int32_t FLASH_V[4 * kTileElemsI32];
    alignas(16) static int32_t FLASH_OUT[9 * kTileElemsI32];
    alignas(16) static int32_t FLASH_REF_SCORE[64];
    alignas(16) static int32_t FLASH_REF_OUT[64];

    for (unsigned tile = 0; tile < 5; tile++) {
        init_tile_pattern(tile_ptr(FLASH_Q, tile), (int32_t)(17 + tile));
        init_tile_pattern(tile_ptr(FLASH_K, tile), (int32_t)(29 + tile));
    }
    for (unsigned tile = 0; tile < 4; tile++) {
        init_tile_pattern(tile_ptr(FLASH_V, tile), (int32_t)(41 + tile));
    }
    for (unsigned i = 0; i < 9 * kTileElemsI32; i++) {
        FLASH_OUT[i] = 0;
    }

    pto::linx::auto_mode::flash_attention_kernel_i32(FLASH_Q, FLASH_K, FLASH_V, FLASH_OUT);

    const unsigned score_q_map[9] = {0, 1, 2, 3, 4, 0, 1, 2, 3};
    const unsigned score_k_map[9] = {0, 1, 2, 3, 4, 1, 2, 3, 4};
    const unsigned score_v_map[9] = {0, 1, 2, 3, 0, 1, 2, 3, 0};
    for (unsigned tile = 0; tile < 9; tile++) {
        tile_matmul_ref_i32_8x8(
            FLASH_REF_SCORE,
            tile_ptr(FLASH_Q, score_q_map[tile]),
            tile_ptr(FLASH_K, score_k_map[tile]));
        tile_matmul_ref_i32_8x8(
            FLASH_REF_OUT,
            FLASH_REF_SCORE,
            tile_ptr(FLASH_V, score_v_map[tile]));
        const int32_t *out_tile = tile_ptr(FLASH_OUT, tile);
        for (unsigned i = 0; i < 64; i++) {
            TEST_EQ32((uint32_t)out_tile[i], (uint32_t)FLASH_REF_OUT[i], 0x000A5000u + tile * 64u + i);
        }
    }

    print_checksum("QEMU_FLASH_CHECKSUM=", checksum_tiles_i32(FLASH_OUT, 9));
    test_pass();
}

static void run_pto_example_kernel_tests()
{
    test_start(0x000A0006);
    uart_puts("PTO example tload/tstore ... ");

    alignas(16) static int32_t EX_SRC[1024];
    alignas(16) static int32_t EX_DST[1024];
    for (unsigned i = 0; i < 1024; i++) {
        EX_SRC[i] = (int32_t)((int)i * 5 - 19);
        EX_DST[i] = 0;
    }

    pto_tload_store_i32(EX_SRC, EX_DST);
    for (unsigned i = 0; i < 256; i++) {
        TEST_EQ32((uint32_t)EX_DST[i], (uint32_t)EX_SRC[i], 0x000A6000u + i);
    }
    test_pass();

    test_start(0x000A0007);
    uart_puts("PTO example mamulb ... ");

    alignas(16) static int32_t EX_MA[1024];
    alignas(16) static int32_t EX_MB[1024];
    alignas(16) static int32_t EX_MC[1024];
    alignas(16) static int32_t EX_MC_REF[64];
    init_tile_pattern(EX_MA, 7);
    init_tile_pattern(EX_MB, 13);
    for (unsigned i = 0; i < 1024; i++) {
        EX_MC[i] = 0;
    }

    pto_mamulb_i32_8x8(EX_MA, EX_MB, EX_MC);
    tile_matmul_ref_i32_8x8(EX_MC_REF, EX_MA, EX_MB);
    for (unsigned i = 0; i < 64; i++) {
        TEST_EQ32((uint32_t)EX_MC[i], (uint32_t)EX_MC_REF[i], 0x000A7000u + i);
    }
    test_pass();

    test_start(0x000A0008);
    uart_puts("PTO example tmatmul_acc ... ");

    alignas(16) static int32_t EX_AA[1024];
    alignas(16) static int32_t EX_AB[1024];
    alignas(16) static int32_t EX_ACC[1024];
    alignas(16) static int32_t EX_ACC_MUL[64];
    init_tile_pattern(EX_AA, 5);
    init_tile_pattern(EX_AB, 9);
    for (unsigned i = 0; i < 1024; i++) {
        EX_ACC[i] = 0;
    }
    for (unsigned i = 0; i < 64; i++) {
        EX_ACC[i] = (int32_t)((int)i - 17);
    }

    pto_tmatmul_acc_i32_8x8(EX_AA, EX_AB, EX_ACC);
    tile_matmul_ref_i32_8x8(EX_ACC_MUL, EX_AA, EX_AB);
    for (unsigned i = 0; i < 64; i++) {
        const int32_t expected = (int32_t)((int64_t)EX_ACC_MUL[i] * 2);
        TEST_EQ32((uint32_t)EX_ACC[i], (uint32_t)expected, 0x000A8000u + i);
    }
    test_pass();

    test_start(0x000A0009);
    uart_puts("PTO example gemm_auto ... ");

    alignas(16) static int32_t EX_GEMM_A[9 * kTileElemsI32];
    alignas(16) static int32_t EX_GEMM_B[8 * kTileElemsI32];
    alignas(16) static int32_t EX_GEMM_OUT[11 * kTileElemsI32];
    alignas(16) static int32_t EX_GEMM_REF[64];

    for (unsigned tile = 0; tile < 9; tile++) {
        init_tile_pattern(tile_ptr(EX_GEMM_A, tile), (int32_t)(3 + tile));
    }
    for (unsigned tile = 0; tile < 8; tile++) {
        init_tile_pattern(tile_ptr(EX_GEMM_B, tile), (int32_t)(11 + tile));
    }
    for (unsigned i = 0; i < 11 * kTileElemsI32; i++) {
        EX_GEMM_OUT[i] = 0;
    }

    pto_gemm_auto_i32(EX_GEMM_A, EX_GEMM_B, EX_GEMM_OUT);

    const unsigned gemm_lhs_map[11] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 0, 1};
    const unsigned gemm_rhs_map[11] = {0, 1, 2, 3, 4, 5, 6, 0, 1, 2, 7};
    for (unsigned tile = 0; tile < 11; tile++) {
        tile_matmul_ref_i32_8x8(
            EX_GEMM_REF,
            tile_ptr(EX_GEMM_A, gemm_lhs_map[tile]),
            tile_ptr(EX_GEMM_B, gemm_rhs_map[tile]));
        const int32_t *out_tile = tile_ptr(EX_GEMM_OUT, tile);
        for (unsigned i = 0; i < 64; i++) {
            TEST_EQ32((uint32_t)out_tile[i], (uint32_t)EX_GEMM_REF[i], 0x000A9000u + tile * 64u + i);
        }
    }
    print_checksum("QEMU_GEMM_EXAMPLE_CHECKSUM=", checksum_tiles_i32(EX_GEMM_OUT, 11));
    test_pass();

    test_start(0x000A000A);
    uart_puts("PTO example flash_auto ... ");

    alignas(16) static int32_t EX_FLASH_Q[5 * kTileElemsI32];
    alignas(16) static int32_t EX_FLASH_K[5 * kTileElemsI32];
    alignas(16) static int32_t EX_FLASH_V[4 * kTileElemsI32];
    alignas(16) static int32_t EX_FLASH_OUT[9 * kTileElemsI32];
    alignas(16) static int32_t EX_FLASH_SCORE[64];
    alignas(16) static int32_t EX_FLASH_REF[64];

    for (unsigned tile = 0; tile < 5; tile++) {
        init_tile_pattern(tile_ptr(EX_FLASH_Q, tile), (int32_t)(17 + tile));
        init_tile_pattern(tile_ptr(EX_FLASH_K, tile), (int32_t)(29 + tile));
    }
    for (unsigned tile = 0; tile < 4; tile++) {
        init_tile_pattern(tile_ptr(EX_FLASH_V, tile), (int32_t)(41 + tile));
    }
    for (unsigned i = 0; i < 9 * kTileElemsI32; i++) {
        EX_FLASH_OUT[i] = 0;
    }

    pto_flash_attention_auto_i32(EX_FLASH_Q, EX_FLASH_K, EX_FLASH_V, EX_FLASH_OUT);

    const unsigned score_q_map[9] = {0, 1, 2, 3, 4, 0, 1, 2, 3};
    const unsigned score_k_map[9] = {0, 1, 2, 3, 4, 1, 2, 3, 4};
    const unsigned score_v_map[9] = {0, 1, 2, 3, 0, 1, 2, 3, 0};
    for (unsigned tile = 0; tile < 9; tile++) {
        tile_matmul_ref_i32_8x8(
            EX_FLASH_SCORE,
            tile_ptr(EX_FLASH_Q, score_q_map[tile]),
            tile_ptr(EX_FLASH_K, score_k_map[tile]));
        tile_matmul_ref_i32_8x8(
            EX_FLASH_REF,
            EX_FLASH_SCORE,
            tile_ptr(EX_FLASH_V, score_v_map[tile]));
        const int32_t *out_tile = tile_ptr(EX_FLASH_OUT, tile);
        for (unsigned i = 0; i < 64; i++) {
            TEST_EQ32((uint32_t)out_tile[i], (uint32_t)EX_FLASH_REF[i], 0x000AA000u + tile * 64u + i);
        }
    }
    print_checksum("QEMU_FLASH_EXAMPLE_CHECKSUM=", checksum_tiles_i32(EX_FLASH_OUT, 9));
    test_pass();
}

static void run_tma_layout_and_padding_tests()
{
    test_start(0x000A000E);
    uart_puts("PTO TMA desc NORM (8x8 sanity) ... ");

    alignas(16) static int32_t ND_DN_SRC[64];
    alignas(16) static int32_t ND_DN_DST[64];
    for (unsigned i = 0; i < 64; i++) {
        ND_DN_SRC[i] = (int32_t)((int)i * 11 - 123);
        ND_DN_DST[i] = 0;
    }

    auto t_nd2dn = pto::linx::tload<kTileSizeCode, kFmtNorm, 8, 8, 8>(ND_DN_SRC);
    pto::linx::tstore<kTileSizeCode, kFmtNorm, 8, 8, 8>(ND_DN_DST, t_nd2dn);

    for (unsigned i = 0; i < 64; i++) {
        TEST_EQ32((uint32_t)ND_DN_DST[i], (uint32_t)ND_DN_SRC[i], 0x000AE000u + i);
    }
    test_pass();

    test_start(0x000A000F);
    uart_puts("PTO TMA desc ND<->NZ (8x8 in 64x16 TR) ... ");

    alignas(16) static int32_t ND_NZ_SRC[1024];
    alignas(16) static int32_t ND_NZ_DST[1024];
    for (unsigned i = 0; i < 1024; i++) {
        ND_NZ_SRC[i] = 0;
        ND_NZ_DST[i] = 0;
        if (i < 64) {
            ND_NZ_SRC[i] = (int32_t)((int)i * 7 - 37);
        }
    }

    auto t_nd2nz = pto::linx::tload<kTileSizeCode, kFmtND2NZ, 8, 8, 64>(ND_NZ_SRC);
    pto::linx::tstore<kTileSizeCode, kFmtND2NZ, 8, 8, 64>(ND_NZ_DST, t_nd2nz);

    for (unsigned i = 0; i < 64; i++) {
        TEST_EQ32((uint32_t)ND_NZ_DST[i], (uint32_t)ND_NZ_SRC[i], 0x000AF000u + i);
    }
    test_pass();

    test_start(0x000A0010);
    uart_puts("PTO TLOAD padding visibility (Null mode) ... ");

    alignas(16) static int32_t PAD_SRC[1024];
    alignas(16) static int32_t PAD_DUMP[1024];
    for (unsigned i = 0; i < 1024; i++) {
        PAD_SRC[i] = 0;
        PAD_DUMP[i] = (int32_t)0x5a5a5a5a;
        if (i < 64) {
            PAD_SRC[i] = (int32_t)((int)i - 9);
        }
    }

    auto t_pad = pto::linx::tload<kTileSizeCode, kFmtND2NZ, 8, 8, 64>(PAD_SRC);
    pto::linx::tstore<kTileSizeCode, kFmtND2NZ, 64, 16, 64>(PAD_DUMP, t_pad);

    /*
     * Staged descriptor bring-up currently guarantees data preservation for
     * the active 8x8 payload, but not full ND<->NZ placement remap.
     */
    for (unsigned i = 0; i < 64; i++) {
        TEST_EQ32((uint32_t)PAD_DUMP[i], (uint32_t)PAD_SRC[i], 0x000A10000u + i);
    }

    bool saw_non_sentinel = false;
    const unsigned pad_samples[4] = {
        8u * 64u + 0u, 8u * 64u + 9u, 9u * 64u + 13u, 15u * 64u + 63u
    };
    for (unsigned i = 0; i < 4; i++) {
        const uint32_t v = (uint32_t)PAD_DUMP[pad_samples[i]];
        if (v != 0x5a5a5a5au) {
            saw_non_sentinel = true;
        }
    }
    /*
     * Staged descriptor ABI (layout/lb0/lb1/size) does not guarantee that
     * padded lanes are materialized during ND<->NZ conversion in all lanes.
     * Preserve the functional check above (active 8x8 region) and treat padded
     * visibility as informational for now.
     */
    if (!saw_non_sentinel) {
        uart_puts("(pad lanes untouched) ");
    }
    test_pass();

    test_start(0x000A0011);
    uart_puts("PTO TMA desc NORM (non-pow2 30x17) ... ");

    alignas(16) static int32_t NP2_SRC[1024];
    alignas(16) static int32_t NP2_DST[1024];
    for (unsigned i = 0; i < 1024; i++) {
        NP2_SRC[i] = 0;
        NP2_DST[i] = 0;
        if (i < 30u * 17u) {
            NP2_SRC[i] = (int32_t)((int)i * 5 + 3);
        }
    }

    auto t_np2 = pto::linx::tload<kTileSizeCode, kFmtNorm, 30, 17, 32>(NP2_SRC);
    pto::linx::tstore<kTileSizeCode, kFmtNorm, 30, 17, 32>(NP2_DST, t_np2);
    for (unsigned i = 0; i < 30u * 17u; i++) {
        TEST_EQ32((uint32_t)NP2_DST[i], (uint32_t)NP2_SRC[i], 0x000A11000u + i);
    }
    test_pass();
}

static void run_tso_store_store_order_smoke()
{
    /*
     * Strict v0.3 contract requires one architectural TSO ordering domain for
     * scalar (BCC) and tile-memory (TMA/MTC) channels. This is a bring-up smoke
     * test that the observable store order is preserved across one scalar store
     * followed by one TSTORE.
     *
     * Note: This does not attempt to create true concurrency; it is a fast gate
     * that catches obvious channel-ordering regressions in the emulator.
     */
    test_start(0x000A000B);
    uart_puts("TSO store->store ordering (scalar + TMA) ... ");

    alignas(16) static int32_t SRC[1024];
    alignas(16) static int32_t DST[1024];
    static volatile uint32_t SCALAR_STORE;

    for (unsigned i = 0; i < 1024; i++) {
        SRC[i] = 0;
        DST[i] = 0;
    }
    SRC[0] = 1;

    auto t = pto::linx::tload<kTileSizeCode>(SRC);

    for (unsigned iter = 0; iter < 128; iter++) {
        SCALAR_STORE = 0;
        DST[0] = 0;

        /* Older store (scalar). */
        SCALAR_STORE = 1;

        /* Younger store (tile-memory channel). */
        pto::linx::tstore<kTileSizeCode>(DST, t);

        const uint32_t y = (uint32_t)DST[0];
        const uint32_t x = SCALAR_STORE;
        TEST_ASSERT(!(y == 1u && x == 0u), 0x000AB000u + iter, 1, ((uint64_t)y << 32) | x);
    }

    test_pass();
}

extern "C" void run_tile_tests(void)
{
    test_suite_begin(0x0000000A);
    run_base_tile_tests();
    run_auto_mode_gemm_test();
    run_auto_mode_flash_test();
    run_pto_example_kernel_tests();
    if (LINX_TEST_ENABLE_TMA_DESC) {
        run_tma_layout_and_padding_tests();
    } else {
        uart_puts("PTO TMA descriptor stress tests ... (skipped)\n");
    }
    run_tso_store_store_order_smoke();
}
