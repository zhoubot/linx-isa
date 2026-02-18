#include <common/pto_tileop.hpp>

using namespace pto;

namespace {

#ifndef PTO_QEMU_SMOKE
#define PTO_QEMU_SMOKE 0
#endif

constexpr int kM = PTO_QEMU_SMOKE ? 16 : 256;
constexpr int kN = PTO_QEMU_SMOKE ? 16 : 256;
constexpr int kK = PTO_QEMU_SMOKE ? 16 : 256;

constexpr int kTM = 16;
constexpr int kTN = 16;
constexpr int kTK = 4;

static_assert(kTM * kTN * kTK * static_cast<int>(sizeof(float)) <= 4096,
              "TMATMUL tile footprint must fit <=4KB");
static_assert(kM % kTM == 0 && kN % kTN == 0 && kK % kTK == 0,
              "global tensor shape must be divisible by tile shape");

using tileA = TileLeft<float, kTM, kTK>;
using tileB = TileRight<float, kTK, kTN>;
using tileAcc = TileAcc<float, kTM, kTN>;
using tileVec = Tile<Location::Vec, float, kTM, kTN, BLayout::RowMajor>;

using gmA = global_tensor<float, RowMajor<kM, kK>>;
using gmB = global_tensor<float, ColMajor<kK, kN>>;
using gmC = global_tensor<float, RowMajor<kM, kN>>;

using itA = global_iterator<gmA, tileA>;
using itB = global_iterator<gmB, tileB>;
using itC = global_iterator<gmC, tileVec>;

} // namespace

extern "C" void gemm_performance_f32(float *lhs_ptr, float *rhs_ptr,
                                        float *dst_ptr, int repeat_tiles) {
  if (repeat_tiles <= 0)
    repeat_tiles = 1;

  itA gA(lhs_ptr);
  itB gB(rhs_ptr);
  itC gC(dst_ptr);

  constexpr int kMTiles = kM / kTM;
  constexpr int kNTiles = kN / kTN;
  constexpr int kKTiles = kK / kTK;

  for (int rep = 0; rep < repeat_tiles; ++rep) {
    for (int mi = 0; mi < kMTiles; ++mi) {
      for (int nj = 0; nj < kNTiles; ++nj) {
        tileA a0;
        tileB b0;
        TLOAD(a0, gA(mi, 0));
        TLOAD(b0, gB(0, nj));

        tileAcc acc;
        TMATMUL(acc, a0, b0);

        for (int kk = 1; kk < kKTiles; ++kk) {
          tileA a;
          tileB b;
          TLOAD(a, gA(mi, kk));
          TLOAD(b, gB(kk, nj));
          TMATMUL_ACC(acc, acc, a, b);
        }

        tileVec out;
        tileVec prev;
        tileVec merged;
        TCVT(out, acc);
        TLOAD(prev, gC(mi, nj));
        TADD(merged, prev, out);
        TSTORE(gC(mi, nj), merged);
      }
    }
  }
}
