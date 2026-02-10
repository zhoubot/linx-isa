#include <pto/linx/TileOps.hpp>

extern "C" void pto_mamulb_i32_8x8(const int *lhs, const int *rhs, int *dst) {
  auto t_lhs = pto::linx::tload<12>(lhs);
  auto t_rhs = pto::linx::tload<12>(rhs);
  auto t_dst = pto::linx::mamulb<8, 8, 8>(t_lhs, t_rhs);
  pto::linx::tstore<12>(dst, t_dst);
}
