#include <stdint.h>

#if defined(PTO_HOST_SIM)
#include <stdio.h>
#else
#include "linx_test.h"
#endif

using usize = __SIZE_TYPE__;

extern "C" void tload_store_i32(int *src_ptr, int *dst_ptr);
extern "C" void mamulb_i32(int *lhs_ptr, int *rhs_ptr, int *dst_ptr);
extern "C" void tmatmul_acc_i32(int *lhs_ptr, int *rhs_ptr, int *dst_ptr);
extern "C" void gemm_i32(int *lhs_ptr, int *rhs_ptr, int *dst_ptr);
extern "C" void gemm_basic_f32(float *lhs_ptr, float *rhs_ptr, float *dst_ptr);
extern "C" void gemm_demo_f32(float *out_ptr, float *a_ptr, float *b_ptr);
extern "C" void gemm_performance_f32(float *lhs_ptr, float *rhs_ptr,
                                        float *dst_ptr, int repeat_tiles);
extern "C" void add_custom_f32(float *x_ptr, float *y_ptr, float *z_ptr);
extern "C" void flash_attention_i32(int *q_ptr, int *k_ptr, int *v_ptr,
                                       int *out_ptr);
extern "C" void flash_attention_demo_f32(float *out_ptr, float *q_ptr,
                                           float *k_ptr, float *v_ptr);
extern "C" void flash_attention_masked_f32(float *out_ptr, float *q_ptr,
                                             float *k_ptr, float *v_ptr);
extern "C" void fa_performance_f32(float *out_ptr, float *q_ptr,
                                     float *k_ptr, float *v_ptr,
                                     int repeat_passes);
extern "C" void mla_attention_demo_f32(float *out_ptr, float *q_ptr,
                                         float *k_ptr, float *v_ptr,
                                         float *wq_ptr, float *wk_ptr,
                                         float *wv_ptr, float *wo_ptr);

#ifndef PTO_QEMU_SMOKE
#define PTO_QEMU_SMOKE 0
#endif

namespace {

static inline uint64_t fnv1a_bytes(const void *ptr, usize bytes) {
  const uint8_t *p = static_cast<const uint8_t *>(ptr);
  uint64_t h = 1469598103934665603ull;
  for (usize i = 0; i < bytes; ++i) {
    h ^= static_cast<uint64_t>(p[i]);
    h *= 1099511628211ull;
  }
  return h;
}

static inline uint32_t lcg32(uint32_t &state) {
  state = state * 1664525u + 1013904223u;
  return state;
}

static void seed_i32(int *buf, usize n, uint32_t seed) {
  uint32_t s = seed;
  for (usize i = 0; i < n; ++i) {
    uint32_t v = lcg32(s);
    buf[i] = static_cast<int32_t>((v & 0x7fffu) - 0x3fffu);
  }
}

static void seed_f32(float *buf, usize n, uint32_t seed) {
  uint32_t s = seed;
  for (usize i = 0; i < n; ++i) {
    uint32_t v = lcg32(s);
    uint32_t m = (v & 0xffffu);
    buf[i] = static_cast<float>(static_cast<int32_t>(m) - 32768) / 8192.0f;
  }
}

static void zero_i32(int *buf, usize n) {
  for (usize i = 0; i < n; ++i)
    buf[i] = 0;
}

static void zero_f32(float *buf, usize n) {
  for (usize i = 0; i < n; ++i)
    buf[i] = 0.0f;
}

#if defined(PTO_HOST_SIM)
static void emit_digest(const char *name, uint64_t digest) {
  printf("PTO_DIGEST %s 0x%016llX\n", name,
         static_cast<unsigned long long>(digest));
}
static void emit_stage(const char *name) { printf("PTO_STAGE %s\n", name); }
#else
static void emit_digest(const char *name, uint64_t digest) {
  uart_puts("PTO_DIGEST ");
  uart_puts(name);
  uart_puts(" 0x");
  uart_puthex64(digest);
  uart_puts("\r\n");
}
static void emit_stage(const char *name) {
  uart_puts("PTO_STAGE ");
  uart_puts(name);
  uart_puts("\r\n");
}
#endif

static void run_all_kernels_emit_digest() {
  emit_stage("begin");
  constexpr usize kMatElems = PTO_QEMU_SMOKE ? 16u * 16u : 256u * 256u;
  constexpr usize kVecElems = PTO_QEMU_SMOKE ? 32u * 32u : 1024u * 1024u;
  constexpr usize kFlashI32Q = PTO_QEMU_SMOKE ? 16u * 4u : 256u * 4u;
  constexpr usize kFlashI32K = PTO_QEMU_SMOKE ? 4u * 16u : 4u * 256u;
  constexpr usize kFlashI32V = PTO_QEMU_SMOKE ? 16u * 16u : 256u * 16u;
  constexpr usize kFlashI32O = PTO_QEMU_SMOKE ? 16u * 16u : 256u * 16u;
  constexpr usize kFlashF32Q = PTO_QEMU_SMOKE ? 16u * 16u : 256u * 16u;
  constexpr usize kFlashF32K = PTO_QEMU_SMOKE ? 16u * 16u : 16u * 256u;
  constexpr usize kFlashF32V = PTO_QEMU_SMOKE ? 16u * 16u : 256u * 16u;
  constexpr usize kFlashF32O = PTO_QEMU_SMOKE ? 16u * 16u : 256u * 16u;
  constexpr usize kFlashMaskQ = PTO_QEMU_SMOKE ? 18u * 16u : 130u * 16u;
  constexpr usize kFlashMaskK = PTO_QEMU_SMOKE ? 16u * 18u : 16u * 130u;
  constexpr usize kFlashMaskV = PTO_QEMU_SMOKE ? 18u * 16u : 130u * 16u;
  constexpr usize kFlashMaskO = PTO_QEMU_SMOKE ? 18u * 16u : 130u * 16u;
  constexpr usize kMlaQ = PTO_QEMU_SMOKE ? 16u * 16u : 256u * 16u;
  constexpr usize kMlaW = 16u * 4u;
  constexpr usize kMlaWo = 4u * 16u;
  constexpr usize kMlaO = PTO_QEMU_SMOKE ? 16u * 16u : 256u * 16u;

  alignas(64) static int iA[kMatElems];
  alignas(64) static int iB[kMatElems];
  alignas(64) static int iC[kMatElems];
  alignas(64) static int iX[kVecElems];
  alignas(64) static int iY[kVecElems];

  alignas(64) static float fA[kMatElems];
  alignas(64) static float fB[kMatElems];
  alignas(64) static float fC[kMatElems];
  alignas(64) static float fX[kVecElems];
  alignas(64) static float fY[kVecElems];
  alignas(64) static float fZ[kVecElems];

  alignas(64) static int flashQ[kFlashI32Q];
  alignas(64) static int flashK[kFlashI32K];
  alignas(64) static int flashV[kFlashI32V];
  alignas(64) static int flashO[kFlashI32O];

  alignas(64) static float flashQf[kFlashF32Q];
  alignas(64) static float flashKf[kFlashF32K];
  alignas(64) static float flashVf[kFlashF32V];
  alignas(64) static float flashOf[kFlashF32O];

  alignas(64) static float flashMaskQ[kFlashMaskQ];
  alignas(64) static float flashMaskK[kFlashMaskK];
  alignas(64) static float flashMaskV[kFlashMaskV];
  alignas(64) static float flashMaskO[kFlashMaskO];

  alignas(64) static float mlaQ[kMlaQ];
  alignas(64) static float mlaK[kMlaQ];
  alignas(64) static float mlaV[kMlaQ];
  alignas(64) static float mlaWq[kMlaW];
  alignas(64) static float mlaWk[kMlaW];
  alignas(64) static float mlaWv[kMlaW];
  alignas(64) static float mlaWo[kMlaWo];
  alignas(64) static float mlaO[kMlaO];

  seed_i32(iA, kMatElems, 0x1001u);
  seed_i32(iB, kMatElems, 0x1002u);
  zero_i32(iC, kMatElems);

  seed_i32(iX, kVecElems, 0x1003u);
  zero_i32(iY, kVecElems);

  seed_f32(fA, kMatElems, 0x2001u);
  seed_f32(fB, kMatElems, 0x2002u);
  zero_f32(fC, kMatElems);

  seed_f32(fX, kVecElems, 0x2003u);
  seed_f32(fY, kVecElems, 0x2004u);
  zero_f32(fZ, kVecElems);

  seed_i32(flashQ, kFlashI32Q, 0x3001u);
  seed_i32(flashK, kFlashI32K, 0x3002u);
  seed_i32(flashV, kFlashI32V, 0x3003u);
  zero_i32(flashO, kFlashI32O);

  seed_f32(flashQf, kFlashF32Q, 0x4001u);
  seed_f32(flashKf, kFlashF32K, 0x4002u);
  seed_f32(flashVf, kFlashF32V, 0x4003u);
  zero_f32(flashOf, kFlashF32O);

  seed_f32(flashMaskQ, kFlashMaskQ, 0x5001u);
  seed_f32(flashMaskK, kFlashMaskK, 0x5002u);
  seed_f32(flashMaskV, kFlashMaskV, 0x5003u);
  zero_f32(flashMaskO, kFlashMaskO);

  seed_f32(mlaQ, kMlaQ, 0x6001u);
  seed_f32(mlaK, kMlaQ, 0x6002u);
  seed_f32(mlaV, kMlaQ, 0x6003u);
  seed_f32(mlaWq, kMlaW, 0x6004u);
  seed_f32(mlaWk, kMlaW, 0x6005u);
  seed_f32(mlaWv, kMlaW, 0x6006u);
  seed_f32(mlaWo, kMlaWo, 0x6007u);
  zero_f32(mlaO, kMlaO);
  emit_stage("seed_done");

  emit_stage("tload_store");
  tload_store_i32(iX, iY);
  emit_digest("tload_store", fnv1a_bytes(iY, sizeof(iY)));

  emit_stage("mamulb");
  mamulb_i32(iA, iB, iC);
  emit_digest("mamulb", fnv1a_bytes(iC, sizeof(iC)));

  zero_i32(iC, kMatElems);
  emit_stage("tmatmul_acc");
  tmatmul_acc_i32(iA, iB, iC);
  emit_digest("tmatmul_acc", fnv1a_bytes(iC, sizeof(iC)));

  zero_i32(iC, kMatElems);
  emit_stage("gemm");
  gemm_i32(iA, iB, iC);
  emit_digest("gemm", fnv1a_bytes(iC, sizeof(iC)));

  zero_f32(fC, kMatElems);
  emit_stage("gemm_basic");
  gemm_basic_f32(fA, fB, fC);
  emit_digest("gemm_basic", fnv1a_bytes(fC, sizeof(fC)));

  zero_f32(fC, kMatElems);
  emit_stage("gemm_demo");
  gemm_demo_f32(fC, fA, fB);
  emit_digest("gemm_demo", fnv1a_bytes(fC, sizeof(fC)));

  zero_f32(fC, kMatElems);
  emit_stage("gemm_performance");
  gemm_performance_f32(fA, fB, fC, PTO_QEMU_SMOKE ? 1 : 2);
  emit_digest("gemm_performance", fnv1a_bytes(fC, sizeof(fC)));

  emit_stage("pre_add_zero");
  zero_f32(fZ, kVecElems);
  emit_stage("add_custom");
  add_custom_f32(fX, fY, fZ);
  emit_digest("add_custom", fnv1a_bytes(fZ, sizeof(fZ)));

  emit_stage("pre_flash_attention");
  emit_stage("flash_attention");
  flash_attention_i32(flashQ, flashK, flashV, flashO);
  emit_digest("flash_attention", fnv1a_bytes(flashO, sizeof(flashO)));

  emit_stage("flash_attention_demo");
  flash_attention_demo_f32(flashOf, flashQf, flashKf, flashVf);
  emit_digest("flash_attention_demo", fnv1a_bytes(flashOf, sizeof(flashOf)));

  emit_stage("flash_attention_masked");
  flash_attention_masked_f32(flashMaskO, flashMaskQ, flashMaskK, flashMaskV);
  emit_digest("flash_attention_masked", fnv1a_bytes(flashMaskO, sizeof(flashMaskO)));

  zero_f32(flashOf, kFlashF32O);
  emit_stage("fa_performance");
  fa_performance_f32(flashOf, flashQf, flashKf, flashVf, PTO_QEMU_SMOKE ? 1 : 2);
  emit_digest("fa_performance", fnv1a_bytes(flashOf, sizeof(flashOf)));

  emit_stage("mla_attention_demo");
  mla_attention_demo_f32(mlaO, mlaQ, mlaK, mlaV, mlaWq, mlaWk, mlaWv, mlaWo);
  emit_digest("mla_attention_demo", fnv1a_bytes(mlaO, sizeof(mlaO)));
  emit_stage("done");
}

} // namespace

#if defined(PTO_HOST_SIM)
int main() {
  run_all_kernels_emit_digest();
  return 0;
}
#else
extern "C" void run_pto_parity_tests(void) {
  test_suite_begin(0x00000010);
  test_start(0x00100001);
  uart_puts("PTO kernel parity digest emission ... ");
  run_all_kernels_emit_digest();
  test_pass();
}
#endif
