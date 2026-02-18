#include <common/pto_tileop.hpp>

using namespace pto;

namespace {

#ifndef PTO_QEMU_SMOKE
#define PTO_QEMU_SMOKE 0
#endif

constexpr int kS = PTO_QEMU_SMOKE ? 16 : 256;
constexpr int kQD = 16;
constexpr int kVD = 16;
constexpr int kTm = 16;
constexpr int kTk = 4;
constexpr float kScale = 0.25f;

static_assert(kTm * kTk * kQD * static_cast<int>(sizeof(float)) <= 4096,
              "QK matmul footprint must fit <=4KB");
static_assert(kTm * kVD * kTk * static_cast<int>(sizeof(float)) <= 4096,
              "WV matmul footprint must fit <=4KB");
static_assert(kS % kTm == 0 && kS % kTk == 0,
              "global sequence shape must be divisible by tile shape");

using gmQ = global_tensor<float, RowMajor<kS, kQD>>;
using gmK = global_tensor<float, ColMajor<kQD, kS>>;
using gmV = global_tensor<float, ColMajor<kS, kVD>>;
using gmO = global_tensor<float, RowMajor<kS, kVD>>;

using tileQ = TileLeft<float, kTm, kQD>;
using tileK = TileRight<float, kQD, kTk>;
using tileWOut = TileAcc<float, kTm, kTk>;
using tileW = Tile<Location::Vec, float, kTm, kTk, BLayout::RowMajor>;
using tileWLeft = TileLeft<float, kTm, kTk>;
using tileV = TileRight<float, kTk, kVD>;
using tileOOut = TileAcc<float, kTm, kVD>;
using tileO = Tile<Location::Vec, float, kTm, kVD, BLayout::RowMajor>;
using tileMax = Tile<Location::Vec, float, kTm, 1, BLayout::RowMajor>;
using tileSum = Tile<Location::Vec, float, kTm, 1, BLayout::RowMajor>;
using tileScaleV = Tile<Location::Vec, float, kTm, 1, BLayout::RowMajor>;

using itQ = global_iterator<gmQ, tileQ>;
using itK = global_iterator<gmK, tileK>;
using itV = global_iterator<gmV, tileV>;
using itO = global_iterator<gmO, tileO>;

} // namespace

extern "C" void flash_attention_demo_f32(float *out_ptr, float *q_ptr,
                                            float *k_ptr, float *v_ptr) {
  itQ gQ(q_ptr);
  itK gK(k_ptr);
  itV gV(v_ptr);
  itO gO(out_ptr);

  constexpr int kQBlocks = kS / kTm;
  constexpr int kKBlocks = kS / kTk;

  for (int i = 0; i < kQBlocks; ++i) {
    tileQ tQ;
    TLOAD(tQ, gQ(i, 0));

    tileMax tMax;
    tileSum tSum(0.0f);
    tileOOut tOOut(0.0f);
    tileO tO(0.0f);
    TEXPANDS(tMax, -1e30f);

    for (int j = 0; j < kKBlocks; ++j) {
      tileK tK;
      tileV tV;
      TLOAD(tK, gK(0, j));
      TLOAD(tV, gV(j, 0));

      tileWOut tWOut;
      tileW tW;
      TMATMUL(tWOut, tQ, tK);
      TCVT(tW, tWOut);
      TMULS(tW, tW, kScale);

      tileMax tLocalMax;
      tileMax tNewMax;
      TROWMAX(tLocalMax, tW);
      TMAX(tNewMax, tMax, tLocalMax);

      tileScaleV tScaleOld;
      tileSum tScaledSum;
      TSUB(tScaleOld, tMax, tNewMax);
      TEXP(tScaleOld, tScaleOld);
      TMUL(tScaledSum, tSum, tScaleOld);

      tileW tNewMaxExpanded;
      TEXPANDCOL(tNewMaxExpanded, tNewMax);
      TSUB(tW, tW, tNewMaxExpanded);
      TEXP(tW, tW);

      tileSum tLocalSum;
      TROWSUM(tLocalSum, tW);
      TADD(tSum, tScaledSum, tLocalSum);

      tileO tScaleOldExpanded;
      TEXPANDCOL(tScaleOldExpanded, tScaleOld);
      TMUL(tO, tO, tScaleOldExpanded);

      tileWLeft tWLeft;
      TCVT(tOOut, tO);
      TCVT(tWLeft, tW);
      MATMACC(tOOut, tWLeft, tV);
      TCVT(tO, tOOut);
      tMax = tNewMax;
    }

    tileSum tInvSum;
    tileO tInvExpanded;
    TRECIP(tInvSum, tSum);
    TEXPANDCOL(tInvExpanded, tInvSum);
    TMUL(tO, tO, tInvExpanded);

    TSTORE(gO(i, 0), tO);
  }
}
