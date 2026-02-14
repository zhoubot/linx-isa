#ifndef PTO_LINX_TILEOPS_HPP
#define PTO_LINX_TILEOPS_HPP

#include <stdint.h>

namespace pto {
namespace linx {

using TileI32 = int __attribute__((vector_size(4096)));

enum : unsigned {
  kTMAFmtNorm  = 0u,
  kTMAFmtND2NZ = 1u,
  kTMAFmtND2ZN = 2u,
  kTMAFmtDN2NZ = 3u,
  kTMAFmtDN2ZN = 4u,
};

enum : unsigned {
  kTMAPadNull = 0u,
  kTMAPadZero = 1u,
  kTMAPadMax  = 2u,
  kTMAPadMin  = 3u,
};

constexpr unsigned make_tma_arg(unsigned format, unsigned pad = kTMAPadNull) {
  return ((pad & 0x3u) << 3) | (format & 0x7u);
}

template <unsigned SizeCode, unsigned Arg = 0, unsigned LB0 = 0, unsigned LB1 = 0,
          unsigned LB2 = 0>
__attribute__((always_inline)) inline TileI32 tload(const void *base) {
  static_assert(SizeCode <= 31, "tload size-code must fit 5 bits");
  static_assert(Arg <= 31, "tload arg must fit 5 bits");
  static_assert(LB0 <= 0x1ffff, "tload LB0 must fit 17 bits");
  static_assert(LB1 <= 0x1ffff, "tload LB1 must fit 17 bits");
  static_assert(LB2 <= 0x1ffff, "tload LB2 must fit 17 bits");
  return __builtin_linx_tma_tload_desc(base, Arg, LB0, LB1, LB2, SizeCode);
}

template <unsigned SizeCode, unsigned Arg = 0, unsigned LB0 = 0, unsigned LB1 = 0,
          unsigned LB2 = 0>
__attribute__((always_inline)) inline void tstore(void *base, TileI32 tile) {
  static_assert(SizeCode <= 31, "tstore size-code must fit 5 bits");
  static_assert(Arg <= 31, "tstore arg must fit 5 bits");
  static_assert(LB0 <= 0x1ffff, "tstore LB0 must fit 17 bits");
  static_assert(LB1 <= 0x1ffff, "tstore LB1 must fit 17 bits");
  static_assert(LB2 <= 0x1ffff, "tstore LB2 must fit 17 bits");
  __builtin_linx_tma_tstore_desc(base, tile, Arg, LB0, LB1, LB2, SizeCode);
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
  return __builtin_linx_cube_mamulb_acc(acc, lhs, rhs, M, N, K);
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

template <unsigned SizeCode>
__attribute__((always_inline)) inline TileI32 tadd(TileI32 lhs, TileI32 rhs) {
  static_assert(SizeCode == 8, "tadd bring-up supports only 4KiB tiles (SizeCode=8)");
  return lhs + rhs;
}

template <unsigned SizeCode>
__attribute__((always_inline)) inline TileI32 tsub(TileI32 lhs, TileI32 rhs) {
  static_assert(SizeCode == 8, "tsub bring-up supports only 4KiB tiles (SizeCode=8)");
  return lhs - rhs;
}

} // namespace linx
} // namespace pto

#endif // PTO_LINX_TILEOPS_HPP
