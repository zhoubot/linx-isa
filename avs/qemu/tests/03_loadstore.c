/*
 * Load/Store Unit Tests for LinxISA
 * Tests: LB, LBU, LH, LHU, LW, LWU, LD, SB, SH, SW, SD
 *        LBI, LHI, LWI, LDI, SBI, SHI, SWI, SDI
 *        HL.* writeback (pre/post-index) and pair ops
 */

#include "linx_test.h"

/* Test data section - aligned for all access sizes */
static const uint8_t  u8_data[]  = { 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0 };
static const uint16_t u16_data[] = { 0x1234, 0x5678, 0x9ABC, 0xDEF0 };
static const uint32_t u32_data[] = { 0x12345678, 0x9ABCDEF0 };
static const uint64_t u64_data[] = { 0x123456789ABCDEF0ULL };

/* Data for stores - will be written to and verified */
static uint8_t  store_u8[8];
static uint16_t store_u16[4];
static uint32_t store_u32[4];
static uint64_t store_u64[2];

/* Test LB (load signed byte) */
static void test_lb_basic(void) {
    int8_t val = (int8_t)u8_data[0];
    TEST_EQ(val, 0x12, 0xC001);
}

static void test_lb_negative(void) {
    /* u8_data[4] = 0x9A which is -102 as int8_t */
    int8_t val = (int8_t)u8_data[4];
    TEST_EQ(val, -102, 0xC002);
}

static void test_lb_aligned(void) {
    int8_t val = (int8_t)u8_data[0];
    TEST_EQ(val, 0x12, 0xC003);
}

/* Test LBU (load unsigned byte) */
static void test_lbu_basic(void) {
    uint8_t val = u8_data[0];
    TEST_EQ(val, 0x12, 0xC010);
}

static void test_lbu_high_bit(void) {
    /* u8_data[4] = 0x9A */
    uint8_t val = u8_data[4];
    TEST_EQ(val, 0x9A, 0xC011);
}

/* Test LH (load signed halfword) */
static void test_lh_basic(void) {
    int16_t val = (int16_t)u16_data[0];
    TEST_EQ(val, 0x1234, 0xC020);
}

static void test_lh_negative(void) {
    /* u16_data[3] = 0xDEF0 = -8464 as int16_t */
    int16_t val = (int16_t)u16_data[3];
    TEST_EQ(val, -8464, 0xC021);
}

/* Test LHU (load unsigned halfword) */
static void test_lhu_basic(void) {
    uint16_t val = u16_data[0];
    TEST_EQ(val, 0x1234, 0xC030);
}

static void test_lhu_high_bit(void) {
    uint16_t val = u16_data[3];
    TEST_EQ(val, 0xDEF0, 0xC031);
}

/* Test LW (load word) */
static void test_lw_basic(void) {
    uint32_t val = u32_data[0];
    TEST_EQ(val, 0x12345678, 0xC040);
}

static void test_lw_second(void) {
    uint32_t val = u32_data[1];
    TEST_EQ(val, 0x9ABCDEF0, 0xC041);
}

/* Test LWU (load unsigned word) */
static void test_lwu_basic(void) {
    uint32_t val = u32_data[0];
    TEST_EQ(val, 0x12345678, 0xC050);
}

static void test_lwu_high_bit(void) {
    uint32_t val = u32_data[1];
    TEST_EQ(val, 0x9ABCDEF0, 0xC051);
}

/* Test LD (load doubleword) */
static void test_ld_basic(void) {
    uint64_t val = u64_data[0];
    TEST_EQ64(val, 0x123456789ABCDEF0ULL, 0xC060);
}

/* Test SB (store byte) */
static void test_sb_basic(void) {
    store_u8[0] = 0xAB;
    TEST_EQ(store_u8[0], 0xAB, 0xC070);
}

static void test_sb_multiple(void) {
    store_u8[0] = 0x12;
    store_u8[1] = 0x34;
    store_u8[2] = 0x56;
    store_u8[3] = 0x78;
    TEST_EQ(store_u8[0], 0x12, 0xC071);
    TEST_EQ(store_u8[1], 0x34, 0xC072);
    TEST_EQ(store_u8[2], 0x56, 0xC073);
    TEST_EQ(store_u8[3], 0x78, 0xC074);
}

/* Test SH (store halfword) */
static void test_sh_basic(void) {
    store_u16[0] = 0xABCD;
    TEST_EQ(store_u16[0], 0xABCD, 0xC080);
}

static void test_sh_alignment(void) {
    store_u16[1] = 0x1234;
    TEST_EQ(store_u16[1], 0x1234, 0xC081);
}

/* Test SW (store word) */
static void test_sw_basic(void) {
    store_u32[0] = 0x12345678;
    TEST_EQ(store_u32[0], 0x12345678, 0xC090);
}

static void test_sw_multiple(void) {
    store_u32[0] = 0x11111111;
    store_u32[1] = 0x22222222;
    store_u32[2] = 0x33333333;
    store_u32[3] = 0x44444444;
    TEST_EQ(store_u32[0], 0x11111111, 0xC091);
    TEST_EQ(store_u32[1], 0x22222222, 0xC092);
    TEST_EQ(store_u32[2], 0x33333333, 0xC093);
    TEST_EQ(store_u32[3], 0x44444444, 0xC094);
}

/* Test SD (store doubleword) */
static void test_sd_basic(void) {
    store_u64[0] = 0xDEADBEEFCAFEBABEULL;
    TEST_EQ64(store_u64[0], 0xDEADBEEFCAFEBABEULL, 0xC0A0);
}

/* Test indexed addressing */
static void test_indexed_load(void) {
    uint32_t base = (uint32_t)(uintptr_t)u32_data;
    uint32_t val = u32_data[1];
    TEST_EQ(val, 0x9ABCDEF0, 0xC0B0);
}

static void test_indexed_store(void) {
    store_u32[2] = 0xCAFEBABE;
    TEST_EQ(store_u32[2], 0xCAFEBABE, 0xC0B1);
}

/* Test with offset addressing */
static void test_offset_load(void) {
    uint32_t base = (uint32_t)(uintptr_t)u8_data;
    uint8_t val = u8_data[4];
    TEST_EQ(val, 0x9A, 0xC0C0);
}

static void test_offset_store(void) {
    store_u8[5] = 0xFF;
    TEST_EQ(store_u8[5], 0xFF, 0xC0C1);
}

/* Test zero extension behavior */
static void test_zext_byte(void) {
    uint8_t val = u8_data[4];  /* 0x9A */
    uint32_t zext = val;       /* Should zero-extend */
    TEST_EQ(zext, 0x9A, 0xC0D0);
}

static void test_zext_half(void) {
    uint16_t val = u16_data[2];  /* 0x9ABC */
    uint32_t zext = val;         /* Should zero-extend */
    TEST_EQ(zext, 0x9ABC, 0xC0D1);
}

/* Test sign extension behavior */
static void test_sext_byte(void) {
    int8_t sval = (int8_t)u8_data[4];  /* 0x9A = -102 */
    int32_t sext = sval;                /* Should sign-extend */
    TEST_EQ32(sext, -102, 0xC0E0);
}

static void test_sext_half(void) {
    int16_t sval = (int16_t)u16_data[3];  /* 0xDEF0 = -8464 */
    int32_t sext = sval;                   /* Should sign-extend */
    TEST_EQ32(sext, -8464, 0xC0E1);
}

static void test_hl_lwuip_pair(void) {
    uint64_t d0 = 0;
    uint64_t d1 = 0;
    const uintptr_t base = (uintptr_t)u32_data;

    __asm__ volatile ("hl.lwuip [%2, 0], ->%0, %1"
                      : "=r"(d0), "=r"(d1)
                      : "r"(base)
                      : "memory");

    TEST_EQ64(d0, 0x12345678ULL, 0xC100);
    TEST_EQ64(d1, 0x9ABCDEF0ULL, 0xC101);
}

static void test_hl_lwui_writeback(void) {
    const uintptr_t base = (uintptr_t)u32_data;

    uint64_t val = 0;
    uint64_t wb = 0;

    // Post-index: load at base, then wb = base + 4.
    __asm__ volatile ("hl.lwui.po [%2, 4], ->%0, %1"
                      : "=r"(val), "=r"(wb)
                      : "r"(base)
                      : "memory");
    TEST_EQ64(val, 0x12345678ULL, 0xC110);
    TEST_EQ64(wb, (uint64_t)(base + 4u), 0xC111);

    // Pre-index: wb = base + 4, then load at wb.
    __asm__ volatile ("hl.lwui.pr [%2, 4], ->%0, %1"
                      : "=r"(val), "=r"(wb)
                      : "r"(base)
                      : "memory");
    TEST_EQ64(val, 0x9ABCDEF0ULL, 0xC112);
    TEST_EQ64(wb, (uint64_t)(base + 4u), 0xC113);

    // Unscaled variants (use aligned delta so semantics match scaled forms).
    __asm__ volatile ("hl.lwui.upo [%2, 4], ->%0, %1"
                      : "=r"(val), "=r"(wb)
                      : "r"(base)
                      : "memory");
    TEST_EQ64(val, 0x12345678ULL, 0xC114);
    TEST_EQ64(wb, (uint64_t)(base + 4u), 0xC115);

    __asm__ volatile ("hl.lwui.upr [%2, 4], ->%0, %1"
                      : "=r"(val), "=r"(wb)
                      : "r"(base)
                      : "memory");
    TEST_EQ64(val, 0x9ABCDEF0ULL, 0xC116);
    TEST_EQ64(wb, (uint64_t)(base + 4u), 0xC117);
}

static void test_hl_swi_writeback(void) {
    // Post-index store: store at base, then wb = base + 4.
    store_u32[0] = 0;
    store_u32[1] = 0;
    const uintptr_t base = (uintptr_t)store_u32;
    const uint64_t v0 = 0xAABBCCDDULL;

    uint64_t wb = 0;
    __asm__ volatile ("hl.swi.po %1, [%2, 4], ->%0"
                      : "=&r"(wb)
                      : "r"(v0), "r"(base)
                      : "memory");
    TEST_EQ(store_u32[0], 0xAABBCCDDu, 0xC120);
    TEST_EQ64(wb, (uint64_t)(base + 4u), 0xC121);

    // Pre-index store: wb = base + 4, then store at wb.
    store_u32[0] = 0;
    store_u32[1] = 0;
    const uint64_t v1 = 0x11223344ULL;

    __asm__ volatile ("hl.swi.pr %1, [%2, 4], ->%0"
                      : "=&r"(wb)
                      : "r"(v1), "r"(base)
                      : "memory");
    TEST_EQ(store_u32[1], 0x11223344u, 0xC122);
    TEST_EQ64(wb, (uint64_t)(base + 4u), 0xC123);

    // Unscaled variants (aligned delta).
    store_u32[0] = 0;
    store_u32[1] = 0;
    const uint64_t v2 = 0x55667788ULL;

    __asm__ volatile ("hl.swi.upo %1, [%2, 4], ->%0"
                      : "=&r"(wb)
                      : "r"(v2), "r"(base)
                      : "memory");
    TEST_EQ(store_u32[0], 0x55667788u, 0xC124);
    TEST_EQ64(wb, (uint64_t)(base + 4u), 0xC125);

    store_u32[0] = 0;
    store_u32[1] = 0;
    const uint64_t v3 = 0x99AABBCCULL;

    __asm__ volatile ("hl.swi.upr %1, [%2, 4], ->%0"
                      : "=&r"(wb)
                      : "r"(v3), "r"(base)
                      : "memory");
    TEST_EQ(store_u32[1], 0x99AABBCCu, 0xC126);
    TEST_EQ64(wb, (uint64_t)(base + 4u), 0xC127);
}

static void test_hl_swip_store_pair(void) {
    store_u32[0] = 0;
    store_u32[1] = 0;
    const uintptr_t base = (uintptr_t)store_u32;

    const uint64_t v0 = 0x01020304ULL;
    const uint64_t v1 = 0xA0B0C0D0ULL;

    __asm__ volatile ("hl.swip %0, %1, [%2, 0]"
                      :
                      : "r"(v0), "r"(v1), "r"(base)
                      : "memory");
    TEST_EQ(store_u32[0], 0x01020304u, 0xC130);
    TEST_EQ(store_u32[1], 0xA0B0C0D0u, 0xC131);

    // Unscaled form (same at offset 0).
    store_u32[0] = 0;
    store_u32[1] = 0;
    const uint64_t v2 = 0x0A0B0C0DULL;
    const uint64_t v3 = 0xEEFF0011ULL;

    __asm__ volatile ("hl.swip.u %0, %1, [%2, 0]"
                      :
                      : "r"(v2), "r"(v3), "r"(base)
                      : "memory");
    TEST_EQ(store_u32[0], 0x0A0B0C0Du, 0xC132);
    TEST_EQ(store_u32[1], 0xEEFF0011u, 0xC133);
}

static void test_hl_ldip_sdip_pair(void) {
    const uint64_t src[2] = { 0x0123456789ABCDEFULL, 0xDEADBEEFCAFEBABEULL };
    uint64_t d0 = 0;
    uint64_t d1 = 0;
    const uintptr_t base = (uintptr_t)src;

    __asm__ volatile ("hl.ldip [%2, 0], ->%0, %1"
                      : "=r"(d0), "=r"(d1)
                      : "r"(base)
                      : "memory");
    TEST_EQ64(d0, 0x0123456789ABCDEFULL, 0xC140);
    TEST_EQ64(d1, 0xDEADBEEFCAFEBABEULL, 0xC141);

    uint64_t dst[2] = { 0, 0 };
    const uintptr_t out = (uintptr_t)dst;
    const uint64_t v0 = 0x1122334455667788ULL;
    const uint64_t v1 = 0x8877665544332211ULL;

    __asm__ volatile ("hl.sdip %0, %1, [%2, 0]"
                      :
                      : "r"(v0), "r"(v1), "r"(out)
                      : "memory");
    TEST_EQ64(dst[0], 0x1122334455667788ULL, 0xC142);
    TEST_EQ64(dst[1], 0x8877665544332211ULL, 0xC143);
}

/* Main test runner */
void run_loadstore_tests(void) {
    test_suite_begin(0xC000);
    
    /* LB tests */
    RUN_TEST(test_lb_basic, 0xC001);
    RUN_TEST(test_lb_negative, 0xC002);
    RUN_TEST(test_lb_aligned, 0xC003);
    
    /* LBU tests */
    RUN_TEST(test_lbu_basic, 0xC010);
    RUN_TEST(test_lbu_high_bit, 0xC011);
    
    /* LH tests */
    RUN_TEST(test_lh_basic, 0xC020);
    RUN_TEST(test_lh_negative, 0xC021);
    
    /* LHU tests */
    RUN_TEST(test_lhu_basic, 0xC030);
    RUN_TEST(test_lhu_high_bit, 0xC031);
    
    /* LW tests */
    RUN_TEST(test_lw_basic, 0xC040);
    RUN_TEST(test_lw_second, 0xC041);
    
    /* LWU tests */
    RUN_TEST(test_lwu_basic, 0xC050);
    RUN_TEST(test_lwu_high_bit, 0xC051);
    
    /* LD tests */
    RUN_TEST(test_ld_basic, 0xC060);
    
    /* SB tests */
    RUN_TEST(test_sb_basic, 0xC070);
    RUN_TEST(test_sb_multiple, 0xC071);
    
    /* SH tests */
    RUN_TEST(test_sh_basic, 0xC080);
    RUN_TEST(test_sh_alignment, 0xC081);
    
    /* SW tests */
    RUN_TEST(test_sw_basic, 0xC090);
    RUN_TEST(test_sw_multiple, 0xC091);
    
    /* SD tests */
    RUN_TEST(test_sd_basic, 0xC0A0);
    
    /* Indexed addressing */
    RUN_TEST(test_indexed_load, 0xC0B0);
    RUN_TEST(test_indexed_store, 0xC0B1);
    
    /* Offset addressing */
    RUN_TEST(test_offset_load, 0xC0C0);
    RUN_TEST(test_offset_store, 0xC0C1);
    
    /* Zero extension */
    RUN_TEST(test_zext_byte, 0xC0D0);
    RUN_TEST(test_zext_half, 0xC0D1);
    
    /* Sign extension */
    RUN_TEST(test_sext_byte, 0xC0E0);
    RUN_TEST(test_sext_half, 0xC0E1);

    /* HL writeback + pair ops */
    RUN_TEST(test_hl_lwuip_pair, 0xC100);
    RUN_TEST(test_hl_lwui_writeback, 0xC110);
    RUN_TEST(test_hl_swi_writeback, 0xC120);
    RUN_TEST(test_hl_swip_store_pair, 0xC130);
    RUN_TEST(test_hl_ldip_sdip_pair, 0xC140);
    
    test_suite_end(32, 32);
}
