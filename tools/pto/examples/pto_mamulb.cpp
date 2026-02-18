#include <common/pto_tileop.hpp>

using namespace pto;

extern "C" void pto_mamulb_i32_8x8(const int *lhs, const int *rhs, int *dst) {
  using tile_i32_8x8 = Tile<Location::Vec, int, 8, 8, BLayout::RowMajor>;

  tile_i32_8x8 t_lhs;
  tile_i32_8x8 t_rhs;
  tile_i32_8x8 t_dst;

  TLOAD(t_lhs, lhs);
  TLOAD(t_rhs, rhs);
  TMATMUL(t_dst, t_lhs, t_rhs);
  TSTORE(dst, t_dst);
}
