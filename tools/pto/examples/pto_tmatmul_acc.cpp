#include <pto/linx/TileOps.hpp>

extern "C" void pto_tmatmul_acc_i32_8x8(const int *lhs, const int *rhs, int *acc_dst) {
  auto t_lhs = pto::linx::tload<12>(lhs);
  auto t_rhs = pto::linx::tload<12>(rhs);
  auto t_acc = pto::linx::tload<12>(acc_dst);
  auto t_out = pto::linx::tmatmul_acc<8, 8, 8>(t_acc, t_lhs, t_rhs);
  pto::linx::tstore<12>(acc_dst, t_out);
}
