#include <common/pto_tileop.hpp>

using namespace pto;

#ifndef PTO_QEMU_SMOKE
#define PTO_QEMU_SMOKE 0
#endif

template <int S, int qD, int vD, int kTm, int kTk>
void flash_attention_masked_frac(float *out_ptr, float *q_ptr, float *k_ptr,
                                 float *v_ptr) {
  static_assert(kTm > 0 && qD > 0 && vD > 0 && kTk > 0,
                "tile dimensions must be positive");
  static_assert(kTm * kTk * qD * static_cast<int>(sizeof(float)) <= 4096,
                "QK matmul footprint must fit <=4KB");
  static_assert(kTm * vD * kTk * static_cast<int>(sizeof(float)) <= 4096,
                "WV matmul footprint must fit <=4KB");

  constexpr float scale = 1.0f / 4.0f;
  constexpr int Qb = S / kTm;
  constexpr int Kb = S / kTk;
  constexpr int rQ = S % kTm;
  constexpr int rK = S % kTk;

  using gmQ = global_tensor<float, RowMajor<S, qD>>;
  using gmK = global_tensor<float, ColMajor<qD, S>>;
  using gmV = global_tensor<float, ColMajor<S, vD>>;
  using gmO = global_tensor<float, RowMajor<S, vD>>;

  using tileQ = TileLeft<float, kTm, qD>;
  using tileK = TileRight<float, qD, kTk>;
  using tileV = TileRight<float, kTk, vD>;
  using tileWOut = TileAcc<float, kTm, kTk>;
  using tileW = Tile<Location::Vec, float, kTm, kTk, BLayout::RowMajor>;
  using tileWLeft = TileLeft<float, kTm, kTk>;

  using tileOOut = TileAcc<float, kTm, vD>;
  using tileO = Tile<Location::Vec, float, kTm, vD, BLayout::RowMajor>;
  using tileMax = Tile<Location::Vec, float, kTm, 1, BLayout::RowMajor>;
  using tileSum = Tile<Location::Vec, float, kTm, 1, BLayout::RowMajor>;
  using tileScale = Tile<Location::Vec, float, kTm, 1, BLayout::RowMajor>;

  using tileQRows = TileLeft<float, kTm, qD, rQ, qD>;
  using tileKCols = TileRight<float, qD, kTk, qD, rK>;
  using tileVRows = TileRight<float, kTk, vD, rK, vD>;

  using tileWOutCols = TileAcc<float, kTm, kTk, kTm, rK>;
  using tileWOutRows = TileAcc<float, kTm, kTk, rQ, kTk>;
  using tileWOutCorner = TileAcc<float, kTm, kTk, rQ, rK>;

  using tileWCols = Tile<Location::Vec, float, kTm, kTk, BLayout::RowMajor,
                          kTm, rK>;
  using tileWRows = Tile<Location::Vec, float, kTm, kTk, BLayout::RowMajor,
                          rQ, kTk>;
  using tileWCorner = Tile<Location::Vec, float, kTm, kTk, BLayout::RowMajor,
                            rQ, rK>;

  using tileWLeftCols = TileLeft<float, kTm, kTk, kTm, rK>;
  using tileWLeftRows = TileLeft<float, kTm, kTk, rQ, kTk>;
  using tileWLeftCorner = TileLeft<float, kTm, kTk, rQ, rK>;

  using tileOOutRows = TileAcc<float, kTm, vD, rQ, vD>;
  using tileORows = Tile<Location::Vec, float, kTm, vD, BLayout::RowMajor,
                          rQ, vD>;
  using tileMaxRows = Tile<Location::Vec, float, kTm, 1, BLayout::RowMajor,
                            rQ, 1>;
  using tileSumRows = Tile<Location::Vec, float, kTm, 1, BLayout::RowMajor,
                            rQ, 1>;
  using tileScaleRows = Tile<Location::Vec, float, kTm, 1, BLayout::RowMajor,
                              rQ, 1>;

  using itQ = global_iterator<gmQ, tileQ>;
  using itK = global_iterator<gmK, tileK>;
  using itV = global_iterator<gmV, tileV>;
  using itO = global_iterator<gmO, tileO>;

  itQ gQ(q_ptr);
  itK gK(k_ptr);
  itV gV(v_ptr);
  itO gO(out_ptr);

  for (int i = 0; i < Qb; ++i) {
    tileQ tQ;
    TLOAD(tQ, gQ(i, 0));

    tileMax tMax;
    tileSum tSum(0.0f);
    tileOOut tOOut(0.0f);
    tileO tO(0.0f);
    TEXPANDS(tMax, -1e30f);

    for (int j = 0; j < Kb; ++j) {
      tileK tK;
      tileV tV;
      TLOAD(tK, gK(0, j));
      TLOAD(tV, gV(j, 0));

      tileWOut tWOut;
      tileW tW;
      tileMax tLocalMax;
      tileMax tNewMax;
      tileScale tScaleOld;
      tileSum tScaledSum;
      tileW tNewMaxExpanded;
      tileSum tLocalSum;
      tileO tScaleOldExpanded;
      tileWLeft tWLeft;

      TMATMUL(tWOut, tQ, tK);
      TCVT(tW, tWOut);
      TMULS(tW, tW, scale);

      TROWMAX(tLocalMax, tW);
      TMAX(tNewMax, tMax, tLocalMax);

      TSUB(tScaleOld, tMax, tNewMax);
      TEXP(tScaleOld, tScaleOld);
      TMUL(tScaledSum, tSum, tScaleOld);

      TEXPANDCOL(tNewMaxExpanded, tNewMax);
      TSUB(tW, tW, tNewMaxExpanded);
      TEXP(tW, tW);

      TROWSUM(tLocalSum, tW);
      TADD(tSum, tScaledSum, tLocalSum);

      TEXPANDCOL(tScaleOldExpanded, tScaleOld);
      TMUL(tO, tO, tScaleOldExpanded);

      TCVT(tOOut, tO);
      TCVT(tWLeft, tW);
      MATMACC(tOOut, tWLeft, tV);
      TCVT(tO, tOOut);
      tMax = tNewMax;
    }

    if constexpr (rK) {
      tileKCols tKTail;
      tileVRows tVTail;
      tileWOutCols tWTailOut;
      tileWCols tWTail;
      tileMax tLocalMax;
      tileMax tNewMax;
      tileScale tScaleOld;
      tileSum tScaledSum;
      tileWCols tNewMaxExpanded;
      tileSum tLocalSum;
      tileO tScaleOldExpanded;
      tileWLeftCols tWLeftTail;

      TLOAD(tKTail, gK(0, Kb));
      TLOAD(tVTail, gV(Kb, 0));

      TMATMUL(tWTailOut, tQ, tKTail);
      TCVT(tWTail, tWTailOut);
      TMULS(tWTail, tWTail, scale);

      TROWMAX(tLocalMax, tWTail);
      TMAX(tNewMax, tMax, tLocalMax);

      TSUB(tScaleOld, tMax, tNewMax);
      TEXP(tScaleOld, tScaleOld);
      TMUL(tScaledSum, tSum, tScaleOld);

      TEXPANDCOL(tNewMaxExpanded, tNewMax);
      TSUB(tWTail, tWTail, tNewMaxExpanded);
      TEXP(tWTail, tWTail);

      TROWSUM(tLocalSum, tWTail);
      TADD(tSum, tScaledSum, tLocalSum);

      TEXPANDCOL(tScaleOldExpanded, tScaleOld);
      TMUL(tO, tO, tScaleOldExpanded);

      TCVT(tOOut, tO);
      TCVT(tWLeftTail, tWTail);
      MATMACC(tOOut, tWLeftTail, tVTail);
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

  if constexpr (rQ) {
    tileQRows tQTail;
    TLOAD(tQTail, gQ(Qb, 0));

    tileMaxRows tMax;
    tileSumRows tSum(0.0f);
    tileOOutRows tOOut(0.0f);
    tileORows tO(0.0f);
    TEXPANDS(tMax, -1e30f);

    for (int j = 0; j < Kb; ++j) {
      tileK tK;
      tileV tV;
      tileWOutRows tWOut;
      tileWRows tW;
      tileMaxRows tLocalMax;
      tileMaxRows tNewMax;
      tileScaleRows tScaleOld;
      tileSumRows tScaledSum;
      tileWRows tNewMaxExpanded;
      tileSumRows tLocalSum;
      tileORows tScaleOldExpanded;
      tileWLeftRows tWLeft;

      TLOAD(tK, gK(0, j));
      TLOAD(tV, gV(j, 0));

      TMATMUL(tWOut, tQTail, tK);
      TCVT(tW, tWOut);
      TMULS(tW, tW, scale);

      TROWMAX(tLocalMax, tW);
      TMAX(tNewMax, tMax, tLocalMax);

      TSUB(tScaleOld, tMax, tNewMax);
      TEXP(tScaleOld, tScaleOld);
      TMUL(tScaledSum, tSum, tScaleOld);

      TEXPANDCOL(tNewMaxExpanded, tNewMax);
      TSUB(tW, tW, tNewMaxExpanded);
      TEXP(tW, tW);

      TROWSUM(tLocalSum, tW);
      TADD(tSum, tScaledSum, tLocalSum);

      TEXPANDCOL(tScaleOldExpanded, tScaleOld);
      TMUL(tO, tO, tScaleOldExpanded);

      TCVT(tOOut, tO);
      TCVT(tWLeft, tW);
      MATMACC(tOOut, tWLeft, tV);
      TCVT(tO, tOOut);
      tMax = tNewMax;
    }

    if constexpr (rK) {
      tileKCols tKTail;
      tileVRows tVTail;
      tileWOutCorner tWOut;
      tileWCorner tW;
      tileMaxRows tLocalMax;
      tileMaxRows tNewMax;
      tileScaleRows tScaleOld;
      tileSumRows tScaledSum;
      tileWCorner tNewMaxExpanded;
      tileSumRows tLocalSum;
      tileORows tScaleOldExpanded;
      tileWLeftCorner tWLeft;

      TLOAD(tKTail, gK(0, Kb));
      TLOAD(tVTail, gV(Kb, 0));

      TMATMUL(tWOut, tQTail, tKTail);
      TCVT(tW, tWOut);
      TMULS(tW, tW, scale);

      TROWMAX(tLocalMax, tW);
      TMAX(tNewMax, tMax, tLocalMax);

      TSUB(tScaleOld, tMax, tNewMax);
      TEXP(tScaleOld, tScaleOld);
      TMUL(tScaledSum, tSum, tScaleOld);

      TEXPANDCOL(tNewMaxExpanded, tNewMax);
      TSUB(tW, tW, tNewMaxExpanded);
      TEXP(tW, tW);

      TROWSUM(tLocalSum, tW);
      TADD(tSum, tScaledSum, tLocalSum);

      TEXPANDCOL(tScaleOldExpanded, tScaleOld);
      TMUL(tO, tO, tScaleOldExpanded);

      TCVT(tOOut, tO);
      TCVT(tWLeft, tW);
      MATMACC(tOOut, tWLeft, tVTail);
      TCVT(tO, tOOut);
      tMax = tNewMax;
    }

    tileSumRows tInvSum;
    tileORows tInvExpanded;
    TRECIP(tInvSum, tSum);
    TEXPANDCOL(tInvExpanded, tInvSum);
    TMUL(tO, tO, tInvExpanded);

    TSTORE(gO(Qb, 0), tO);
  }
}

extern "C" void flash_attention_masked_f32(float *out_ptr, float *q_ptr,
                                              float *k_ptr, float *v_ptr) {
  if (PTO_QEMU_SMOKE)
    flash_attention_masked_frac<18, 16, 16, 16, 4>(out_ptr, q_ptr, k_ptr,
                                                    v_ptr);
  else
    flash_attention_masked_frac<130, 16, 16, 16, 4>(out_ptr, q_ptr, k_ptr,
                                                     v_ptr);
}
