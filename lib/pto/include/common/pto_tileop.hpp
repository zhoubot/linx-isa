#ifndef PTO_COMMON_PTO_TILEOP_HPP
#define PTO_COMMON_PTO_TILEOP_HPP

#include <stdint.h>
#include <type_traits>

#include <pto/linx/impl/backend.hpp>

namespace pto {

enum class Location : uint8_t {
  Vec,
  Left,
  Right,
  Acc,
};

enum class BLayout : uint8_t {
  RowMajor = 0,
  ColMajor = 1,
};

template <int Rows_, int Cols_>
struct RowMajor {
  static constexpr int Rows = Rows_;
  static constexpr int Cols = Cols_;
  static constexpr bool IsRowMajor = true;
};

template <int Rows_, int Cols_>
struct ColMajor {
  static constexpr int Rows = Rows_;
  static constexpr int Cols = Cols_;
  static constexpr bool IsRowMajor = false;
};

template <typename Element_, typename Layout_>
struct global_tensor {
  using DType = Element_;
  using Layout = Layout_;
};

namespace detail {

using ptrdiff_builtin_t = __PTRDIFF_TYPE__;

template <typename... Ts>
using void_t = void;

// TMA format selectors used by B.ARG in strict v0.3.
constexpr long long kLayoutNorm = 0ll;     // NORM.normal
constexpr long long kLayoutND2NZ = 2ll;    // ND2NZ.normal
constexpr long long kLayoutND2ZN = 3ll;    // ND2ZN.normal
constexpr long long kLayoutDN2ZN = 8ll;    // DN2ZN.normal
constexpr long long kLayoutDN2NZ = 9ll;    // DN2NZ.normal

template <typename TileT>
constexpr unsigned tileBytes() {
  constexpr int rows = TileT::Rows;
  constexpr int cols = TileT::Cols;
  constexpr unsigned bytes =
      static_cast<unsigned>(rows * cols * sizeof(typename TileT::DType));
  static_assert(bytes > 0u, "PTO Linx strict-v0.3: tile bytes must be positive");
  return bytes;
}

template <typename TileT>
constexpr unsigned tileSizeCode() {
  static_assert(tileBytes<TileT>() <= linx::detail::kMaxTileBytes,
                "PTO Linx strict-v0.3: tile size exceeds 4KB");
  // Keep a single 4KB size profile in PR5 user-facing wrappers to avoid
  // cross-op metadata skew while strict Tile SSA balancing is enabled.
  return 8u;
}

template <typename TileT>
constexpr unsigned tileDTypeCode() {
  return linx::detail::DTypeCode<typename TileT::DType>::value;
}

template <typename TileT>
constexpr long long tileLayoutCode() {
  return TileT::LayoutTag == BLayout::RowMajor ? 0ll : 1ll;
}

template <typename GTensor>
constexpr long long gmStrideBytes() {
  constexpr long long elemBytes =
      static_cast<long long>(sizeof(typename GTensor::DType));
  if constexpr (GTensor::Layout::IsRowMajor)
    return static_cast<long long>(GTensor::Layout::Cols) * elemBytes;
  return static_cast<long long>(GTensor::Layout::Rows) * elemBytes;
}

template <typename GTensor, typename TileT>
constexpr long long tensorTileLayoutCode() {
  if constexpr (TileT::Loc == Location::Left || TileT::Loc == Location::Acc) {
    return GTensor::Layout::IsRowMajor ? kLayoutND2ZN : kLayoutDN2ZN;
  }
  if constexpr (TileT::Loc == Location::Right) {
    return GTensor::Layout::IsRowMajor ? kLayoutND2NZ : kLayoutDN2NZ;
  }
  return kLayoutNorm;
}

template <typename TileT>
constexpr long long tileLB0() {
  return TileT::RowValid > 0 ? static_cast<long long>(TileT::RowValid)
                             : static_cast<long long>(TileT::Rows);
}

template <typename TileT>
constexpr long long tileLB1() {
  return TileT::ColValid > 0 ? static_cast<long long>(TileT::ColValid)
                             : static_cast<long long>(TileT::Cols);
}

template <typename GTensor, typename TileT>
inline ptrdiff_builtin_t tileOffset(int tileRow, int tileCol) {
  const int row = tileRow * TileT::Rows;
  const int col = tileCol * TileT::Cols;
  if constexpr (GTensor::Layout::IsRowMajor) {
    return static_cast<ptrdiff_builtin_t>(row) * GTensor::Layout::Cols + col;
  }
  return static_cast<ptrdiff_builtin_t>(col) * GTensor::Layout::Rows + row;
}

template <typename AddressLike>
inline auto addressPtr(const AddressLike &addr) -> decltype(addr.ptr()) {
  return addr.ptr();
}

template <typename T>
inline T *addressPtr(T *addr) {
  return addr;
}

template <typename T>
inline const T *addressPtr(const T *addr) {
  return addr;
}

template <typename AddressLike, typename TileT, typename = void>
struct AddressDesc {
  static constexpr long long Layout = tileLayoutCode<TileT>();
  static constexpr long long LB0 = tileLB0<TileT>();
  static constexpr long long LB1 = tileLB1<TileT>();
  static constexpr long long StrideBytes = 0ll;
};

template <typename AddressLike, typename TileT>
struct AddressDesc<AddressLike, TileT,
                   void_t<decltype(AddressLike::kLayoutCode),
                          decltype(AddressLike::kLB0),
                          decltype(AddressLike::kLB1),
                          decltype(AddressLike::kStrideBytes)>> {
  static constexpr long long Layout = AddressLike::kLayoutCode;
  static constexpr long long LB0 = AddressLike::kLB0;
  static constexpr long long LB1 = AddressLike::kLB1;
  static constexpr long long StrideBytes = AddressLike::kStrideBytes;
};

template <typename AddressLike, typename TileT>
constexpr long long addressLayoutCode() {
  return AddressDesc<AddressLike, TileT>::Layout;
}

template <typename AddressLike, typename TileT>
constexpr long long addressLB0() {
  return AddressDesc<AddressLike, TileT>::LB0;
}

template <typename AddressLike, typename TileT>
constexpr long long addressLB1() {
  return AddressDesc<AddressLike, TileT>::LB1;
}

template <typename AddressLike, typename TileT>
constexpr long long addressStrideBytes() {
  return AddressDesc<AddressLike, TileT>::StrideBytes;
}

} // namespace detail

template <Location Loc_, typename Element_, int Rows_, int Cols_,
          BLayout Layout_ = BLayout::RowMajor, int RowValid_ = Rows_,
          int ColValid_ = Cols_>
struct Tile {
  using DType = Element_;
  using RawTile = linx::detail::RawTile;

  static constexpr Location Loc = Loc_;
  static constexpr int Rows = Rows_;
  static constexpr int Cols = Cols_;
  static constexpr int RowValid = RowValid_;
  static constexpr int ColValid = ColValid_;
  static constexpr BLayout LayoutTag = Layout_;

  Tile() = default;

  template <typename Scalar>
  explicit Tile(Scalar scalar) {
    raw_ = linx::detail::teplSplat<0x045u, detail::tileSizeCode<Tile>(),
                                   detail::tileDTypeCode<Tile>(), 2u>(scalar);
  }

  RawTile &raw() { return raw_; }
  const RawTile &raw() const { return raw_; }

private:
  RawTile raw_{};
};

template <typename Element_, int Rows_, int Cols_, int RowValid_ = Rows_,
          int ColValid_ = Cols_>
using TileLeft =
    Tile<Location::Left, Element_, Rows_, Cols_, BLayout::ColMajor, RowValid_,
         ColValid_>;

template <typename Element_, int Rows_, int Cols_, int RowValid_ = Rows_,
          int ColValid_ = Cols_>
using TileRight =
    Tile<Location::Right, Element_, Rows_, Cols_, BLayout::RowMajor, RowValid_,
         ColValid_>;

template <typename Element_, int Rows_, int Cols_, int RowValid_ = Rows_,
          int ColValid_ = Cols_>
using TileAcc =
    Tile<Location::Acc, Element_, Rows_, Cols_, BLayout::ColMajor, RowValid_,
         ColValid_>;

template <typename GTensor, typename TileT>
class global_iterator {
public:
  using Element = typename GTensor::DType;

  explicit global_iterator(Element *base) : base_(base) {}

  struct tile_address {
    using TensorType = GTensor;
    using TileType = TileT;
    static constexpr long long kLayoutCode =
        detail::tensorTileLayoutCode<GTensor, TileT>();
    // TMA contract: LB0/LB1 are GM-side inner/outer counts.
    // ND(row-major): inner=cols, outer=rows; DN(column-major): inner=rows, outer=cols.
    static constexpr long long kLB0 =
        GTensor::Layout::IsRowMajor ? detail::tileLB1<TileT>()
                                    : detail::tileLB0<TileT>();
    static constexpr long long kLB1 =
        GTensor::Layout::IsRowMajor ? detail::tileLB0<TileT>()
                                    : detail::tileLB1<TileT>();
    static constexpr long long kStrideBytes = detail::gmStrideBytes<GTensor>();

    Element *base;
    int tileRow;
    int tileCol;

    Element *ptr() const {
      return base + detail::tileOffset<GTensor, TileT>(tileRow, tileCol);
    }
  };

  tile_address operator()(int tileRow, int tileCol) const {
    return tile_address{base_, tileRow, tileCol};
  }

private:
  Element *base_;
};

namespace tepl {
constexpr unsigned TADD = 0x000u;
constexpr unsigned TSUB = 0x001u;
constexpr unsigned TMUL = 0x002u;
constexpr unsigned TMAX = 0x004u;
constexpr unsigned TCVT = 0x00fu;
constexpr unsigned TROWMAX = 0x020u;
constexpr unsigned TROWSUM = 0x022u;
constexpr unsigned TCOLEXPAND = 0x027u;
constexpr unsigned TEXP = 0x040u;
constexpr unsigned TRECIP = 0x044u;
constexpr unsigned TEXPANDS = 0x045u;
} // namespace tepl

// Core tile ops used by PR5 FlashAttention bring-up.
template <typename DstTile, typename SrcAddress>
inline void TLOAD(DstTile &dst, const SrcAddress &src) {
  dst.raw() = linx::detail::tileTLoad<detail::tileSizeCode<DstTile>(),
                                      detail::tileDTypeCode<DstTile>(),
                                      detail::addressLayoutCode<SrcAddress, DstTile>(),
                                      detail::addressLB0<SrcAddress, DstTile>(),
                                      detail::addressLB1<SrcAddress, DstTile>(),
                                      detail::addressStrideBytes<SrcAddress, DstTile>()>(
      reinterpret_cast<const void *>(detail::addressPtr(src)));
}

template <typename DstAddress, typename SrcTile>
inline void TSTORE(const DstAddress &dst, SrcTile &src) {
  linx::detail::tileTStore<detail::tileSizeCode<SrcTile>(),
                           detail::tileDTypeCode<SrcTile>(),
                           detail::addressLayoutCode<DstAddress, SrcTile>(),
                           detail::addressLB0<DstAddress, SrcTile>(),
                           detail::addressLB1<DstAddress, SrcTile>(),
                           detail::addressStrideBytes<DstAddress, SrcTile>()>(
      reinterpret_cast<void *>(detail::addressPtr(dst)), src.raw());
}

template <typename DstTile, typename SrcTile>
inline void TMOV(DstTile &dst, const SrcTile &src, unsigned mode = 0u) {
  if (mode == 1u) {
    dst.raw() = linx::detail::tileTMov<detail::tileSizeCode<DstTile>(),
                                       detail::tileDTypeCode<DstTile>(),
                                       detail::tileLayoutCode<DstTile>(), 1u, 1u>(
        src.raw());
  } else {
    dst.raw() = linx::detail::tileTMov<detail::tileSizeCode<DstTile>(),
                                       detail::tileDTypeCode<DstTile>(),
                                       detail::tileLayoutCode<DstTile>(), 1u, 0u>(
        src.raw());
  }
}

template <typename TileRes, typename TileLeft_, typename TileRight_>
inline void TMATMUL(TileRes &dst, const TileLeft_ &lhs, const TileRight_ &rhs) {
  // Strict-v0.3 compiler policy:
  // tile_bytes = ceil(m*n*k*elem_bits/8) must fit <=4KB
  // (m=Rows, n=Cols, k=lhs.Cols).
  constexpr unsigned M = static_cast<unsigned>(TileRes::Rows);
  constexpr unsigned N = static_cast<unsigned>(TileRes::Cols);
  constexpr unsigned K = static_cast<unsigned>(TileLeft_::Cols);
  dst.raw() = linx::detail::cubeMamulb<M, N, K>(lhs.raw(), rhs.raw());
}

template <typename TileRes, typename TileLeft_, typename TileRight_>
inline void TMATMUL_ACC(TileRes &dst, TileRes &acc, const TileLeft_ &lhs,
                        const TileRight_ &rhs) {
  constexpr unsigned M = static_cast<unsigned>(TileRes::Rows);
  constexpr unsigned N = static_cast<unsigned>(TileRes::Cols);
  constexpr unsigned K = static_cast<unsigned>(TileLeft_::Cols);
  dst.raw() = linx::detail::cubeMamulbAcc<M, N, K>(acc.raw(), lhs.raw(), rhs.raw());
}

template <typename TileRes, typename TileLeft_, typename TileRight_>
inline void MATMACC(TileRes &dst, const TileLeft_ &lhs, const TileRight_ &rhs) {
  // Keep strict CUBE accumulator-chain legality: materialize the product with
  // TMATMUL, then accumulate explicitly with TEPL add.
  TileRes product;
  TMATMUL(product, lhs, rhs);
  TADD(dst, dst, product);
}

template <typename DstTile, typename SrcTile>
inline void TCVT(DstTile &dst, const SrcTile &src) {
  dst.raw() = linx::detail::teplUnary<tepl::TCVT, detail::tileSizeCode<DstTile>(),
                                      detail::tileDTypeCode<DstTile>()>(
      src.raw());
}

template <typename DstTile, typename SrcTile0, typename SrcTile1>
inline void TADD(DstTile &dst, const SrcTile0 &src0, const SrcTile1 &src1) {
  dst.raw() = linx::detail::teplBinary<tepl::TADD, detail::tileSizeCode<DstTile>(),
                                       detail::tileDTypeCode<DstTile>()>(
      src0.raw(), src1.raw());
}

template <typename DstTile, typename SrcTile0, typename SrcTile1>
inline void TSUB(DstTile &dst, const SrcTile0 &src0, const SrcTile1 &src1) {
  dst.raw() = linx::detail::teplBinary<tepl::TSUB, detail::tileSizeCode<DstTile>(),
                                       detail::tileDTypeCode<DstTile>()>(
      src0.raw(), src1.raw());
}

template <typename DstTile, typename SrcTile0, typename SrcTile1>
inline void TMUL(DstTile &dst, const SrcTile0 &src0, const SrcTile1 &src1) {
  dst.raw() = linx::detail::teplBinary<tepl::TMUL, detail::tileSizeCode<DstTile>(),
                                       detail::tileDTypeCode<DstTile>()>(
      src0.raw(), src1.raw());
}

template <typename DstTile, typename SrcTile0, typename SrcTile1>
inline void TMAX(DstTile &dst, const SrcTile0 &src0, const SrcTile1 &src1) {
  dst.raw() = linx::detail::teplBinary<tepl::TMAX, detail::tileSizeCode<DstTile>(),
                                       detail::tileDTypeCode<DstTile>()>(
      src0.raw(), src1.raw());
}

template <typename DstTile, typename SrcTile>
inline void TROWMAX(DstTile &dst, const SrcTile &src) {
  dst.raw() = linx::detail::teplUnary<tepl::TROWMAX,
                                      detail::tileSizeCode<DstTile>(),
                                      detail::tileDTypeCode<DstTile>()>(src.raw());
}

template <typename DstTile, typename SrcTile>
inline void TROWSUM(DstTile &dst, const SrcTile &src) {
  dst.raw() = linx::detail::teplUnary<tepl::TROWSUM,
                                      detail::tileSizeCode<DstTile>(),
                                      detail::tileDTypeCode<DstTile>()>(src.raw());
}

template <typename DstTile, typename SrcTile>
inline void TEXP(DstTile &dst, const SrcTile &src) {
  dst.raw() = linx::detail::teplUnary<tepl::TEXP, detail::tileSizeCode<DstTile>(),
                                      detail::tileDTypeCode<DstTile>()>(
      src.raw());
}

template <typename DstTile, typename SrcTile>
inline void TRECIP(DstTile &dst, const SrcTile &src) {
  dst.raw() = linx::detail::teplUnary<tepl::TRECIP,
                                      detail::tileSizeCode<DstTile>(),
                                      detail::tileDTypeCode<DstTile>()>(src.raw());
}

template <typename DstTile, typename SrcTile, typename Scalar>
inline void TMULS(DstTile &dst, const SrcTile &src, Scalar scalar) {
  dst.raw() = linx::detail::teplBinaryScalar<tepl::TMUL,
                                             detail::tileSizeCode<DstTile>(),
                                             detail::tileDTypeCode<DstTile>(), 1u>(
      src.raw(), scalar);
}

template <typename DstTile, typename Scalar>
inline void TEXPANDS(DstTile &dst, Scalar scalar) {
  dst.raw() = linx::detail::teplSplat<tepl::TEXPANDS,
                                      detail::tileSizeCode<DstTile>(),
                                      detail::tileDTypeCode<DstTile>(), 2u>(scalar);
}

template <typename DstTile, typename SrcTile>
inline void TCOLEXPAND(DstTile &dst, const SrcTile &src) {
  dst.raw() = linx::detail::teplUnary<tepl::TCOLEXPAND,
                                      detail::tileSizeCode<DstTile>(),
                                      detail::tileDTypeCode<DstTile>()>(
      src.raw());
}

template <typename DstTile, typename SrcTile>
inline void TEXPANDCOL(DstTile &dst, const SrcTile &src) {
  TCOLEXPAND(dst, src);
}

} // namespace pto

#endif // PTO_COMMON_PTO_TILEOP_HPP
