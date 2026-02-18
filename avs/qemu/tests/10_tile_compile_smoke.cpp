#include "linx_test.h"

#include <stdint.h>

extern "C" void tload_store_i32(int *src, int *dst);
extern "C" void mamulb_i32(int *lhs, int *rhs, int *dst);
extern "C" void tmatmul_acc_i32(int *lhs, int *rhs, int *acc_dst);
extern "C" void gemm_i32(int *lhs, int *rhs, int *dst);
extern "C" void flash_attention_i32(int *query, int *key, int *value, int *dst);
extern "C" void flash_attention_masked_f32(float *out_ptr, float *q_ptr,
                                           float *k_ptr, float *v_ptr);

extern "C" void run_tile_tests(void) {
  test_suite_begin(0x0000000A);
  test_start(0x000AFFF0);
  uart_puts("Tile compile smoke ... ");

  alignas(16) static int32_t A[1024];
  alignas(16) static int32_t B[1024];
  alignas(16) static int32_t C[1024];

  alignas(16) static int32_t GEMM_A[9 * 1024];
  alignas(16) static int32_t GEMM_B[8 * 1024];
  alignas(16) static int32_t GEMM_O[11 * 1024];

  alignas(16) static int32_t FLASH_Q[5 * 1024];
  alignas(16) static int32_t FLASH_K[5 * 1024];
  alignas(16) static int32_t FLASH_V[4 * 1024];
  alignas(16) static int32_t FLASH_O[9 * 1024];

  alignas(16) static float FM_Q[1024];
  alignas(16) static float FM_K[1024];
  alignas(16) static float FM_V[1024];
  alignas(16) static float FM_O[1024];

  tload_store_i32(A, C);
  mamulb_i32(A, B, C);
  tmatmul_acc_i32(A, B, C);
  gemm_i32(GEMM_A, GEMM_B, GEMM_O);
  flash_attention_i32(FLASH_Q, FLASH_K, FLASH_V, FLASH_O);
  flash_attention_masked_f32(FM_O, FM_Q, FM_K, FM_V);

  test_pass();
}
