#include <common/pto_tileop.hpp>

using namespace pto;

extern "C" void pto_tmatmul_acc_i32_8x8(const int *lhs, const int *rhs,
                                        int *acc_dst) {
  using tile_i32_8x8 = Tile<Location::Vec, int, 8, 8, BLayout::RowMajor>;

  tile_i32_8x8 t_lhs;
  tile_i32_8x8 t_rhs;
  tile_i32_8x8 t_acc;
  tile_i32_8x8 t_out;

  TLOAD(t_lhs, lhs);
  TLOAD(t_rhs, rhs);
  TMATMUL(t_acc, t_lhs, t_rhs);
  TMATMUL_ACC(t_out, t_acc, t_lhs, t_rhs);
  TSTORE(acc_dst, t_out);
}
