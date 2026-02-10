#ifndef PTO_LINX_TILEOPS_HPP
#define PTO_LINX_TILEOPS_HPP

#include <stdint.h>

namespace pto {
namespace linx {

using TileI32 = int __attribute__((vector_size(4096)));

template <unsigned SizeCode>
__attribute__((always_inline)) inline TileI32 tload(const void *base) {
  static_assert(SizeCode <= 31, "tload size-code must fit 5 bits");
  return __builtin_linx_tma_tload(base, SizeCode);
}

template <unsigned SizeCode>
__attribute__((always_inline)) inline void tstore(void *base, TileI32 tile) {
  static_assert(SizeCode <= 31, "tstore size-code must fit 5 bits");
  __builtin_linx_tma_tstore(base, tile, SizeCode);
}

template <unsigned M, unsigned N, unsigned K>
__attribute__((always_inline)) inline TileI32 mamulb(TileI32 lhs, TileI32 rhs) {
  static_assert(M <= 255 && N <= 255 && K <= 255,
                "mamulb dimensions must fit immediate fields");
  return __builtin_linx_cube_mamulb(lhs, rhs, M, N, K);
}

template <unsigned M, unsigned N, unsigned K>
__attribute__((always_inline)) inline TileI32 tmatmul(TileI32 lhs, TileI32 rhs) {
  return mamulb<M, N, K>(lhs, rhs);
}

template <unsigned M, unsigned N, unsigned K>
__attribute__((always_inline)) inline TileI32 tmatmul_acc(TileI32 acc, TileI32 lhs, TileI32 rhs) {
  (void)acc;
  return mamulb<M, N, K>(lhs, rhs);
}

template <unsigned M, unsigned N, unsigned K>
__attribute__((always_inline)) inline TileI32 tmatmul_mx(TileI32 lhs, TileI32 rhs) {
#if defined(PTO_LINX_ENABLE_TMATMUL_MX) && PTO_LINX_ENABLE_TMATMUL_MX
  return mamulb<M, N, K>(lhs, rhs);
#else
  (void)lhs;
  (void)rhs;
  __builtin_trap();
#endif
}

} // namespace linx
} // namespace pto

#endif // PTO_LINX_TILEOPS_HPP
