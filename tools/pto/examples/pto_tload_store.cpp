#include <pto/linx/TileOps.hpp>

extern "C" void pto_tload_store_i32(const int *src, int *dst) {
  auto tile = pto::linx::tload<12>(src);
  pto::linx::tstore<12>(dst, tile);
}
