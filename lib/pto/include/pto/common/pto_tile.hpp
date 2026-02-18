/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef PTO_TILE_HPP
#define PTO_TILE_HPP

#include "pto/common/memory.hpp"
#include <pto/common/type.hpp>
#include <pto/common/constants.hpp>
#ifdef __CPU_SIM
#include <iomanip>
#endif

namespace pto {

enum class Layout {
    ND, // ND RowMajor
    DN, // DN ColMajor
    NZ, // NZ for cube
    SCALE,
    MX_A_ND,
    MX_A_DN,
    MX_A_ZZ,
    MX_B_ND,
    MX_B_DN,
    MX_B_NN,
    NC1HWC0,
    NCHW,
    NHWC,
    FRACTAL_Z,
    FRACTAL_Z_S16S8,
    MAX,
};
namespace GlobalTensorDim {
constexpr int DIM_0 = 0;
constexpr int DIM_1 = 1;
constexpr int DIM_2 = 2;
constexpr int DIM_3 = 3;
constexpr int DIM_4 = 4;
constexpr int TOTAL_DIM = 5;
} // namespace GlobalTensorDim

constexpr int DYNAMIC = -1;

template <int N1 = DYNAMIC, int N2 = DYNAMIC, int N3 = DYNAMIC, int N4 = DYNAMIC, int N5 = DYNAMIC>
struct Shape {
    static constexpr int staticShape[5] = {N1, N2, N3, N4, N5};
    PTO_INTERNAL Shape(int n1, int n2, int n3, int n4, int n5)
    {
        if constexpr (N1 == DYNAMIC) shape[GlobalTensorDim::DIM_0] = n1;
        if constexpr (N2 == DYNAMIC) shape[GlobalTensorDim::DIM_1] = n2;
        if constexpr (N3 == DYNAMIC) shape[GlobalTensorDim::DIM_2] = n3;
        if constexpr (N4 == DYNAMIC) shape[GlobalTensorDim::DIM_3] = n4;
        if constexpr (N5 == DYNAMIC) shape[GlobalTensorDim::DIM_4] = n5;
    }

    PTO_INTERNAL Shape() {
        if constexpr (N1 == DYNAMIC) shape[GlobalTensorDim::DIM_0] = 1;
        if constexpr (N2 == DYNAMIC) shape[GlobalTensorDim::DIM_1] = 1;
        if constexpr (N3 == DYNAMIC) shape[GlobalTensorDim::DIM_2] = 1;
        if constexpr (N4 == DYNAMIC) shape[GlobalTensorDim::DIM_3] = 1;
        if constexpr (N5 == DYNAMIC) shape[GlobalTensorDim::DIM_4] = 1;
    }

	    PTO_INTERNAL Shape(int n) {
	        static_assert((N1 == DYNAMIC) + (N2 == DYNAMIC) + (N3 == DYNAMIC) + (N4 == DYNAMIC) + (N5 == DYNAMIC) ==
	                          GlobalTensorDim::DIM_1,
	            "1-parameter constructor is only applicable to Shape with 1 dynamic dimension.");
	        if constexpr (N1 == DYNAMIC) shape[GlobalTensorDim::DIM_0] = n;
	        else if constexpr (N2 == DYNAMIC) shape[GlobalTensorDim::DIM_1] = n;
	        else if constexpr (N3 == DYNAMIC) shape[GlobalTensorDim::DIM_2] = n;
        else if constexpr (N4 == DYNAMIC) shape[GlobalTensorDim::DIM_3] = n;
        else if constexpr (N5 == DYNAMIC) shape[GlobalTensorDim::DIM_4] = n;
    }

	    PTO_INTERNAL Shape(int n1, int n2) {
	        static_assert((N1 == DYNAMIC) + (N2 == DYNAMIC) + (N3 == DYNAMIC) + (N4 == DYNAMIC) + (N5 == DYNAMIC) ==
	                          GlobalTensorDim::DIM_2,
	            "2-parameter constructor is only applicable to Shape with 2 dynamic dimensions.");

	        int idx = 0;
	        const int vals[] = {n1, n2};
	        if constexpr (N1 == DYNAMIC) shape[GlobalTensorDim::DIM_0] = vals[idx++];
        if constexpr (N2 == DYNAMIC) shape[GlobalTensorDim::DIM_1] = vals[idx++];
        if constexpr (N3 == DYNAMIC) shape[GlobalTensorDim::DIM_2] = vals[idx++];
        if constexpr (N4 == DYNAMIC) shape[GlobalTensorDim::DIM_3] = vals[idx++];
        if constexpr (N5 == DYNAMIC) shape[GlobalTensorDim::DIM_4] = vals[idx++];
    }

    PTO_INTERNAL Shape(int n1, int n2, int n3) {
        static_assert((N1 == DYNAMIC) + (N2 == DYNAMIC) + (N3 == DYNAMIC) + (N4 == DYNAMIC) + (N5 == DYNAMIC) ==
                          GlobalTensorDim::DIM_3,
            "3-parameter constructor is only applicable to Shape with 3 dynamic dimensions.");
        int idx = 0;
        const int vals[] = {n1, n2, n3};
        if constexpr (N1 == DYNAMIC) shape[GlobalTensorDim::DIM_0] = vals[idx++];
        if constexpr (N2 == DYNAMIC) shape[GlobalTensorDim::DIM_1] = vals[idx++];
        if constexpr (N3 == DYNAMIC) shape[GlobalTensorDim::DIM_2] = vals[idx++];
        if constexpr (N4 == DYNAMIC) shape[GlobalTensorDim::DIM_3] = vals[idx++];
        if constexpr (N5 == DYNAMIC) shape[GlobalTensorDim::DIM_4] = vals[idx++];
    }

    PTO_INTERNAL Shape(int n1, int n2, int n3, int n4) {
        static_assert((N1 == DYNAMIC) + (N2 == DYNAMIC) + (N3 == DYNAMIC) + (N4 == DYNAMIC) + (N5 == DYNAMIC) ==
                          GlobalTensorDim::DIM_4,
            "4-parameter constructor is only applicable to Shape with 4 dynamic dimensions.");
        int idx = 0;
        const int vals[] = {n1, n2, n3, n4};
        if constexpr (N1 == DYNAMIC) shape[GlobalTensorDim::DIM_0] = vals[idx++];
        if constexpr (N2 == DYNAMIC) shape[GlobalTensorDim::DIM_1] = vals[idx++];
        if constexpr (N3 == DYNAMIC) shape[GlobalTensorDim::DIM_2] = vals[idx++];
        if constexpr (N4 == DYNAMIC) shape[GlobalTensorDim::DIM_3] = vals[idx++];
        if constexpr (N5 == DYNAMIC) shape[GlobalTensorDim::DIM_4] = vals[idx++];
    }

public:
    int shape[GlobalTensorDim::TOTAL_DIM] = {1};
};

template <int SN1 = DYNAMIC, int SN2 = DYNAMIC, int SN3 = DYNAMIC, int SN4 = DYNAMIC, int SN5 = DYNAMIC>
struct Stride {
    static constexpr int staticStride[GlobalTensorDim::TOTAL_DIM] = {SN1, SN2, SN3, SN4, SN5};
    PTO_INTERNAL Stride(int n1, int n2, int n3, int n4, int n5)
    {
        if constexpr (SN1 == DYNAMIC) stride[GlobalTensorDim::DIM_0] = n1;
        if constexpr (SN2 == DYNAMIC) stride[GlobalTensorDim::DIM_1] = n2;
        if constexpr (SN3 == DYNAMIC) stride[GlobalTensorDim::DIM_2] = n3;
        if constexpr (SN4 == DYNAMIC) stride[GlobalTensorDim::DIM_3] = n4;
        if constexpr (SN5 == DYNAMIC) stride[GlobalTensorDim::DIM_4] = n5;
    }

    PTO_INTERNAL Stride() {
        if constexpr (SN1 == DYNAMIC) stride[GlobalTensorDim::DIM_0] = 1;
        if constexpr (SN2 == DYNAMIC) stride[GlobalTensorDim::DIM_1] = 1;
        if constexpr (SN3 == DYNAMIC) stride[GlobalTensorDim::DIM_2] = 1;
        if constexpr (SN4 == DYNAMIC) stride[GlobalTensorDim::DIM_3] = 1;
        if constexpr (SN5 == DYNAMIC) stride[GlobalTensorDim::DIM_4] = 1;
    }

    PTO_INTERNAL Stride(int n) {
        static_assert((SN1 == DYNAMIC) + (SN2 == DYNAMIC) + (SN3 == DYNAMIC) + (SN4 == DYNAMIC) + (SN5 == DYNAMIC) ==
                          GlobalTensorDim::DIM_1,
            "1-parameter constructor is only applicable to Stride with 1 dynamic dimension.");

        if constexpr (SN1 == DYNAMIC) stride[GlobalTensorDim::DIM_0] = n;
        else if constexpr (SN2 == DYNAMIC) stride[GlobalTensorDim::DIM_1] = n;
        else if constexpr (SN3 == DYNAMIC) stride[GlobalTensorDim::DIM_2] = n;
        else if constexpr (SN4 == DYNAMIC) stride[GlobalTensorDim::DIM_3] = n;
        else if constexpr (SN5 == DYNAMIC) stride[GlobalTensorDim::DIM_4] = n;
    }

    PTO_INTERNAL Stride(int n1, int n2) {
        static_assert((SN1 == DYNAMIC) + (SN2 == DYNAMIC) + (SN3 == DYNAMIC) + (SN4 == DYNAMIC) + (SN5 == DYNAMIC) ==
                          GlobalTensorDim::DIM_2,
            "2-parameter constructor is only applicable to Stride with 2 dynamic dimensions.");
        int idx = 0;
        const int vals[] = {n1, n2};
        if constexpr (SN1 == DYNAMIC) stride[GlobalTensorDim::DIM_0] = vals[idx++];
        if constexpr (SN2 == DYNAMIC) stride[GlobalTensorDim::DIM_1] = vals[idx++];
        if constexpr (SN3 == DYNAMIC) stride[GlobalTensorDim::DIM_2] = vals[idx++];
        if constexpr (SN4 == DYNAMIC) stride[GlobalTensorDim::DIM_3] = vals[idx++];
        if constexpr (SN5 == DYNAMIC) stride[GlobalTensorDim::DIM_4] = vals[idx++];
    }

    PTO_INTERNAL Stride(int n1, int n2, int n3) {
        static_assert((SN1 == DYNAMIC) + (SN2 == DYNAMIC) + (SN3 == DYNAMIC) + (SN4 == DYNAMIC) + (SN5 == DYNAMIC) ==
                          GlobalTensorDim::DIM_3,
            "3-parameter constructor is only applicable to Stride with 3 dynamic dimensions.");
        int idx = 0;
        const int vals[] = {n1, n2, n3};
        if constexpr (SN1 == DYNAMIC) stride[GlobalTensorDim::DIM_0] = vals[idx++];
        if constexpr (SN2 == DYNAMIC) stride[GlobalTensorDim::DIM_1] = vals[idx++];
        if constexpr (SN3 == DYNAMIC) stride[GlobalTensorDim::DIM_2] = vals[idx++];
        if constexpr (SN4 == DYNAMIC) stride[GlobalTensorDim::DIM_3] = vals[idx++];
        if constexpr (SN5 == DYNAMIC) stride[GlobalTensorDim::DIM_4] = vals[idx++];
    }

    PTO_INTERNAL Stride(int n1, int n2, int n3, int n4) {
        static_assert((SN1 == DYNAMIC) + (SN2 == DYNAMIC) + (SN3 == DYNAMIC) + (SN4 == DYNAMIC) + (SN5 == DYNAMIC) ==
                          GlobalTensorDim::DIM_4,
            "4-parameter constructor is only applicable to Stride with 4 dynamic dimensions.");
        int idx = 0;
        const int vals[] = {n1, n2, n3, n4};
        if constexpr (SN1 == DYNAMIC) stride[GlobalTensorDim::DIM_0] = vals[idx++];
        if constexpr (SN2 == DYNAMIC) stride[GlobalTensorDim::DIM_1] = vals[idx++];
        if constexpr (SN3 == DYNAMIC) stride[GlobalTensorDim::DIM_2] = vals[idx++];
        if constexpr (SN4 == DYNAMIC) stride[GlobalTensorDim::DIM_3] = vals[idx++];
        if constexpr (SN5 == DYNAMIC) stride[GlobalTensorDim::DIM_4] = vals[idx++];
    }

public:
    int stride[GlobalTensorDim::TOTAL_DIM] = {1};
};

template <typename Element_, typename Shape_, typename Stride_, Layout Layout_ = Layout::ND>
struct GlobalTensor {
    using Shape = Shape_;
    using Stride = Stride_;
    using RawDType = Element_;
    using DType = __gm__ Element_;
    static constexpr Layout layout = Layout_;

    static const Shape defaultShape;
    static const Stride defaultStride;

    static constexpr int staticShape[GlobalTensorDim::TOTAL_DIM] = {Shape::staticShape[GlobalTensorDim::DIM_0],
        Shape::staticShape[GlobalTensorDim::DIM_1], Shape::staticShape[GlobalTensorDim::DIM_2],
        Shape::staticShape[GlobalTensorDim::DIM_3], Shape::staticShape[GlobalTensorDim::DIM_4]};
    static constexpr int staticStride[GlobalTensorDim::TOTAL_DIM] = {Stride::staticStride[GlobalTensorDim::DIM_0],
        Stride::staticStride[GlobalTensorDim::DIM_1], Stride::staticStride[GlobalTensorDim::DIM_2],
        Stride::staticStride[GlobalTensorDim::DIM_3], Stride::staticStride[GlobalTensorDim::DIM_4]};
    PTO_INTERNAL GlobalTensor(
        DType *data, const Shape &shape = defaultShape, const Stride &stride = defaultStride)
    {
        data_ = data;

        if constexpr (staticShape[GlobalTensorDim::DIM_0] == DYNAMIC) {
            shape_.shape[GlobalTensorDim::DIM_0] = shape.shape[GlobalTensorDim::DIM_0];
        }
        if constexpr (staticShape[GlobalTensorDim::DIM_1] == DYNAMIC) {
            shape_.shape[GlobalTensorDim::DIM_1] = shape.shape[GlobalTensorDim::DIM_1];
        }
        if constexpr (staticShape[GlobalTensorDim::DIM_2] == DYNAMIC) {
            shape_.shape[GlobalTensorDim::DIM_2] = shape.shape[GlobalTensorDim::DIM_2];
        }
        if constexpr (staticShape[GlobalTensorDim::DIM_3] == DYNAMIC) {
            shape_.shape[GlobalTensorDim::DIM_3] = shape.shape[GlobalTensorDim::DIM_3];
        }
        if constexpr (staticShape[GlobalTensorDim::DIM_4] == DYNAMIC) {
            shape_.shape[GlobalTensorDim::DIM_4] = shape.shape[GlobalTensorDim::DIM_4];
        }

        if constexpr (staticStride[GlobalTensorDim::DIM_0] == DYNAMIC) {
            stride_.stride[GlobalTensorDim::DIM_0] = stride.stride[GlobalTensorDim::DIM_0];
        }
        if constexpr (staticStride[GlobalTensorDim::DIM_1] == DYNAMIC) {
            stride_.stride[GlobalTensorDim::DIM_1] = stride.stride[GlobalTensorDim::DIM_1];
        }
        if constexpr (staticStride[GlobalTensorDim::DIM_2] == DYNAMIC) {
            stride_.stride[GlobalTensorDim::DIM_2] = stride.stride[GlobalTensorDim::DIM_2];
        }
        if constexpr (staticStride[GlobalTensorDim::DIM_3] == DYNAMIC) {
            stride_.stride[GlobalTensorDim::DIM_3] = stride.stride[GlobalTensorDim::DIM_3];
        }
        if constexpr (staticStride[GlobalTensorDim::DIM_4] == DYNAMIC) {
            stride_.stride[GlobalTensorDim::DIM_4] = stride.stride[GlobalTensorDim::DIM_4];
        }
    }

    PTO_INTERNAL int GetShape(const int dim)
    {
        switch (dim) {
            case GlobalTensorDim::DIM_0: return GetShapeSize<staticShape[GlobalTensorDim::DIM_0]>(dim);
            case GlobalTensorDim::DIM_1: return GetShapeSize<staticShape[GlobalTensorDim::DIM_1]>(dim);
            case GlobalTensorDim::DIM_2: return GetShapeSize<staticShape[GlobalTensorDim::DIM_2]>(dim);
            case GlobalTensorDim::DIM_3: return GetShapeSize<staticShape[GlobalTensorDim::DIM_3]>(dim);
            case GlobalTensorDim::DIM_4: return GetShapeSize<staticShape[GlobalTensorDim::DIM_4]>(dim);
            default: return -1;
        }
    }

    PTO_INTERNAL int GetStride(const int dim)
    {
        switch (dim) {
            case GlobalTensorDim::DIM_0: return GetStrideSize<staticStride[GlobalTensorDim::DIM_0]>(dim);
            case GlobalTensorDim::DIM_1: return GetStrideSize<staticStride[GlobalTensorDim::DIM_1]>(dim);
            case GlobalTensorDim::DIM_2: return GetStrideSize<staticStride[GlobalTensorDim::DIM_2]>(dim);
            case GlobalTensorDim::DIM_3: return GetStrideSize<staticStride[GlobalTensorDim::DIM_3]>(dim);
            case GlobalTensorDim::DIM_4: return GetStrideSize<staticStride[GlobalTensorDim::DIM_4]>(dim);
            default: return -1;
        }
    }

    template <int dim>
    AICORE static constexpr int GetShape()
    {
        static_assert(dim >= GlobalTensorDim::DIM_0 && dim < GlobalTensorDim::TOTAL_DIM, "only support get dim(0-4)");
        if constexpr (dim == GlobalTensorDim::DIM_0) {
            static_assert(staticShape[GlobalTensorDim::DIM_0] != DYNAMIC,
                "dim 0 is dynamic, cannot be obtained using the template interface.");
            return staticShape[GlobalTensorDim::DIM_0];
        }
        if constexpr (dim == GlobalTensorDim::DIM_1) {
            static_assert(staticShape[GlobalTensorDim::DIM_1] != DYNAMIC,
                "dim 1 is dynamic, cannot be obtained using the template interface.");
            return staticShape[GlobalTensorDim::DIM_1];
        }
        if constexpr (dim == GlobalTensorDim::DIM_2) {
            static_assert(staticShape[GlobalTensorDim::DIM_2] != DYNAMIC,
                "dim 2 is dynamic, cannot be obtained using the template interface.");
            return staticShape[GlobalTensorDim::DIM_2];
        }
        if constexpr (dim == GlobalTensorDim::DIM_3) {
            static_assert(staticShape[GlobalTensorDim::DIM_3] != DYNAMIC,
                "dim 3 is dynamic, cannot be obtained using the template interface.");
            return staticShape[GlobalTensorDim::DIM_3];
        }
        if constexpr (dim == GlobalTensorDim::DIM_4) {
            static_assert(staticShape[GlobalTensorDim::DIM_4] != DYNAMIC,
                "dim 4 is dynamic, cannot be obtained using the template interface.");
            return staticShape[GlobalTensorDim::DIM_4];
        }
        return -1;
    }

    template <int dim>
    AICORE static constexpr int GetStride()
    {
        static_assert(dim >= GlobalTensorDim::DIM_0 && dim < GlobalTensorDim::TOTAL_DIM, "only support get dim(0-4)");
        if constexpr (dim == GlobalTensorDim::DIM_0) {
            static_assert(staticStride[GlobalTensorDim::DIM_0] != DYNAMIC,
                "dim 0 is dynamic, cannot be obtained using the template interface.");
            return staticStride[GlobalTensorDim::DIM_0];
        }
        if constexpr (dim == GlobalTensorDim::DIM_1) {
            static_assert(staticStride[GlobalTensorDim::DIM_1] != DYNAMIC,
                "dim 1 is dynamic, cannot be obtained using the template interface.");
            return staticStride[GlobalTensorDim::DIM_1];
        }
        if constexpr (dim == GlobalTensorDim::DIM_2) {
            static_assert(staticStride[GlobalTensorDim::DIM_2] != DYNAMIC,
                "dim 2 is dynamic, cannot be obtained using the template interface.");
            return staticStride[GlobalTensorDim::DIM_2];
        }
        if constexpr (dim == GlobalTensorDim::DIM_3) {
            static_assert(staticStride[GlobalTensorDim::DIM_3] != DYNAMIC,
                "dim 3 is dynamic, cannot be obtained using the template interface.");
            return staticStride[GlobalTensorDim::DIM_3];
        }
        if constexpr (dim == GlobalTensorDim::DIM_4) {
            static_assert(staticStride[GlobalTensorDim::DIM_4] != DYNAMIC,
                "dim 4 is dynamic, cannot be obtained using the template interface.");
            return staticStride[GlobalTensorDim::DIM_4];
        }
        return -1;
    }

    template <typename T, typename AddrType>
    friend AICORE void TASSIGN_IMPL(T &src, AddrType addr);

    AICORE DType *data()
    {
        return data_;
    }

private:
    template <int StaticShape>
    PTO_INTERNAL int GetShapeSize(const int dim)
    {
        if constexpr (StaticShape == DYNAMIC) {
            return shape_.shape[dim];
        } else {
            return StaticShape;
        }
    }

    template <int StaticStride>
    PTO_INTERNAL int GetStrideSize(const int dim)
    {
        if constexpr (StaticStride == DYNAMIC) {
            return stride_.stride[dim];
        } else {
            return StaticStride;
        }
    }

    AICORE void SetAddr(DType *addr) { data_ = addr; }

    DType *data_;
    Shape shape_ = defaultShape;
    Stride stride_ = defaultStride;
};

template <typename Element_, typename Shape_, typename Stride_, Layout Layout_>
const typename GlobalTensor<Element_, Shape_, Stride_, Layout_>::Shape
GlobalTensor<Element_, Shape_, Stride_, Layout_>::defaultShape{1, 1, 1, 1, 1};

template <typename Element_, typename Shape_, typename Stride_, Layout Layout_>
const typename GlobalTensor<Element_, Shape_, Stride_, Layout_>::Stride
GlobalTensor<Element_, Shape_, Stride_, Layout_>::defaultStride{1, 1, 1, 1, 1};

template <typename T, int rows = DYNAMIC, int cols = DYNAMIC, Layout Layout_ = Layout::ND>
struct TileShape2D;

template <typename T, int cols>
constexpr int GetTileShape2DNZCols()
{
    if constexpr (cols == DYNAMIC) {
        return DYNAMIC;
    } else {
        return static_cast<int>(cols / (C0_SIZE_BYTE / sizeof(T)));
    }
}

template <typename T, int rows>
constexpr int GetTileShape2DNZRows()
{
    if constexpr (rows == DYNAMIC) {
        return DYNAMIC;
    } else {
        return static_cast<int>(rows / FRACTAL_NZ_ROW);
    }
}

template <typename T, int rows, int cols>
struct TileShape2D<T, rows, cols, Layout::NZ>
    : public Shape<1, GetTileShape2DNZCols<T, cols>(), GetTileShape2DNZRows<T, rows>(), FRACTAL_NZ_ROW,
                   C0_SIZE_BYTE / sizeof(T)> {
    static constexpr int C0Size = C0_SIZE_BYTE / sizeof(T);
    using Parent = Shape<1, GetTileShape2DNZCols<T, cols>(),
                         GetTileShape2DNZRows<T, rows>(), FRACTAL_NZ_ROW, C0Size>;

    static_assert((rows == DYNAMIC) || (rows % FRACTAL_NZ_ROW == 0), "rows must be divisible by 16 for Layout::NZ");
    static_assert((cols == DYNAMIC) || (cols % C0Size == 0), "cols must be divisible by C0Size for Layout::NZ");

    PTO_INTERNAL TileShape2D() : Parent() {}

    PTO_INTERNAL TileShape2D(int dynamicRows, int dynamicCols)
        : Parent(1, dynamicCols / C0Size, dynamicRows / FRACTAL_NZ_ROW, FRACTAL_NZ_ROW, C0Size)
    {
    }
    using Parent::Parent;
};

template <typename T, int cols>
constexpr int GetShape2DCols()
{
    if constexpr (cols == DYNAMIC) {
        return DYNAMIC;
    } else {
        return cols;
    }
}
template <typename T, int rows>
constexpr int GetShape2DRows()
{
    if constexpr (rows == DYNAMIC) {
        return DYNAMIC;
    } else {
        return rows;
    }
}
template <typename T, int rows, int cols>
struct TileShape2D<T, rows, cols, Layout::ND>
    : public Shape<1, 1, 1, GetShape2DRows<T, rows>(),
                   GetShape2DCols<T, cols>()> {
    using Parent = Shape<1, 1, 1, GetShape2DRows<T, rows>(),
                         GetShape2DCols<T, cols>()>;

    PTO_INTERNAL TileShape2D() : Parent() {}

    PTO_INTERNAL TileShape2D(int dynamicRows, int dynamicCols) : Parent(1, 1, 1, dynamicRows, dynamicCols) {}
    using Parent::Parent;
};
template <typename T, int rows, int cols>
struct TileShape2D<T, rows, cols, Layout::DN>
    : public Shape<1, 1, 1, GetShape2DRows<T, rows>(),
                   GetShape2DCols<T, cols>()> {
    using Parent = Shape<1, 1, 1, GetShape2DRows<T, rows>(),
                         GetShape2DCols<T, cols>()>;

    PTO_INTERNAL TileShape2D() : Parent() {}

    PTO_INTERNAL TileShape2D(int dynamicRows, int dynamicCols) : Parent(1, 1, 1, dynamicRows, dynamicCols) {}
    using Parent::Parent;
};

template <typename T, int rows = DYNAMIC, int cols = DYNAMIC, Layout Layout_ = Layout::ND>
struct BaseShape2D;

template <typename T, int cols>
constexpr int GetBaseShape2DNZCols()
{
    if constexpr (cols == DYNAMIC) {
        return DYNAMIC;
    } else {
        return static_cast<int>(cols / (C0_SIZE_BYTE / sizeof(T)));
    }
}

template <typename T, int rows, int cols>
constexpr int GetBaseShape2DStride0()
{
    if constexpr (cols == DYNAMIC || rows == DYNAMIC) {
        return DYNAMIC;
    } else {
        return static_cast<int>(cols * rows);
    }
}
template <typename T, int rows>
constexpr int GetBaseShape2DStride1()
{
    if constexpr (rows == DYNAMIC) {
        return DYNAMIC;
    } else {
        return static_cast<int>(rows * (C0_SIZE_BYTE / sizeof(T)));
    }
}
template <typename T, int rows, int cols>
struct BaseShape2D<T, rows, cols, Layout::NZ>
    : public Stride<GetBaseShape2DStride0<T, rows, cols>(),
                    GetBaseShape2DStride1<T, rows>(),
                    FRACTAL_NZ_ROW * (C0_SIZE_BYTE / sizeof(T)), C0_SIZE_BYTE / sizeof(T), 1> {
    static constexpr int C0Size = C0_SIZE_BYTE / sizeof(T);
    static constexpr int FractalNZSize = FRACTAL_NZ_ROW * (C0_SIZE_BYTE / sizeof(T));
    using Parent = Stride<GetBaseShape2DStride0<T, rows, cols>(),
                          GetBaseShape2DStride1<T, rows>(), FractalNZSize, C0Size, 1>;
    static_assert((rows == DYNAMIC) || (rows % FRACTAL_NZ_ROW == 0), "rows must be divisible by 16 for Layout::NZ");
    static_assert((cols == DYNAMIC) || (cols % C0Size == 0), "cols must be divisible by C0Size for Layout::NZ");

    PTO_INTERNAL BaseShape2D() : Parent() {}

    PTO_INTERNAL BaseShape2D(int dynamicRows, int dynamicCols)
        : Parent(dynamicCols * dynamicRows, dynamicRows * C0Size, FractalNZSize, C0Size, 1)
    {
    }
    using Parent::Parent;
};
template <typename T, int rows, int cols>
struct BaseShape2D<T, rows, cols, Layout::ND>
    : public Stride<GetBaseShape2DStride0<T, rows, cols>(),
                    GetBaseShape2DStride0<T, rows, cols>(),
                    GetBaseShape2DStride0<T, rows, cols>(),
                    GetShape2DCols<T, cols>(), 1> {
    using Parent = Stride<GetBaseShape2DStride0<T, rows, cols>(),
                          GetBaseShape2DStride0<T, rows, cols>(),
                          GetBaseShape2DStride0<T, rows, cols>(),
                          GetShape2DCols<T, cols>(), 1>;

    PTO_INTERNAL BaseShape2D() : Parent() {}

    PTO_INTERNAL BaseShape2D(int dynamicRows, int dynamicCols)
        : Parent(dynamicRows * dynamicCols, dynamicRows * dynamicCols, dynamicRows * dynamicCols, dynamicCols, 1)
    {
    }
    using Parent::Parent;
};
template <typename T, int rows, int cols>
struct BaseShape2D<T, rows, cols, Layout::DN>
    : public Stride<GetBaseShape2DStride0<T, rows, cols>(),
                    GetBaseShape2DStride0<T, rows, cols>(),
                    GetBaseShape2DStride0<T, rows, cols>(), 1,
                    GetShape2DRows<T, rows>()> {
    using Parent = Stride<GetBaseShape2DStride0<T, rows, cols>(),
                          GetBaseShape2DStride0<T, rows, cols>(),
                          GetBaseShape2DStride0<T, rows, cols>(), 1,
                          GetShape2DRows<T, rows>()>;

    PTO_INTERNAL BaseShape2D() : Parent() {}

    PTO_INTERNAL BaseShape2D(int dynamicRows, int dynamicCols)
        : Parent(dynamicRows * dynamicCols, dynamicRows * dynamicCols, dynamicRows * dynamicCols, 1, dynamicRows)
    {
    }
    using Parent::Parent;
};

template <typename T, int rows, int cols>
struct BaseShape2D<T, rows, cols, Layout::MX_A_ZZ>
    : public Stride<GetBaseShape2DStride0<T, rows, cols>(), (cols == DYNAMIC) ? DYNAMIC : cols * 16, 32, 2, 1> {
    static constexpr int FractalSize = 32;
    using Parent =
        Stride<GetBaseShape2DStride0<T, rows, cols>(), (cols == DYNAMIC) ? DYNAMIC : cols * 16, FractalSize, 2, 1>;

    PTO_INTERNAL BaseShape2D() : Parent() {}

    PTO_INTERNAL BaseShape2D(int dynamicRows, int dynamicCols)
        : Parent(dynamicCols * dynamicRows, dynamicCols * 16, FractalSize, 2, 1)
    {
    }
    using Parent::Parent;
};

template <typename T, int rows, int cols>
struct TileShape2D<T, rows, cols, Layout::MX_A_ZZ>
    : public Shape<1, rows == DYNAMIC ? DYNAMIC : rows / 16, cols == DYNAMIC ? DYNAMIC : cols / 2, 16, 2> {
    using Parent = Shape<1, rows == DYNAMIC ? DYNAMIC : rows / 16, cols == DYNAMIC ? DYNAMIC : cols / 2, 16, 2>;
    static constexpr int C0Size = 2;
    static_assert((cols == DYNAMIC) || (cols % C0Size == 0), "cols must be divisible by C0Size for Layout::MX_A_ZZ");
    PTO_INTERNAL TileShape2D() : Parent() {}
    PTO_INTERNAL TileShape2D(int dynamicRows, int dynamicCols) : Parent(1, dynamicRows / 16, dynamicCols / 2, 16, 2) {}
    using Parent::Parent;
};

template <typename T, int rows, int cols>
struct BaseShape2D<T, rows, cols, Layout::MX_A_ND>
    : public Stride<GetBaseShape2DStride0<T, rows, cols>(), GetBaseShape2DStride0<T, rows, cols>(), cols, 2, 1> {
    using Parent = Stride<GetBaseShape2DStride0<T, rows, cols>(), GetBaseShape2DStride0<T, rows, cols>(), cols, 2, 1>;

    PTO_INTERNAL BaseShape2D() : Parent() {}
    PTO_INTERNAL BaseShape2D(int dynamicRows, int dynamicCols)
        : Parent(dynamicCols * dynamicRows, dynamicCols * dynamicRows, dynamicCols, 2, 1)
    {
    }
    using Parent::Parent;
};

template <typename T, int rows, int cols>
struct TileShape2D<T, rows, cols, Layout::MX_A_ND>
    : public Shape<1, 1, rows == DYNAMIC ? DYNAMIC : rows, cols == DYNAMIC ? DYNAMIC : cols / 2, 2> {
    using Parent = Shape<1, 1, rows == DYNAMIC ? DYNAMIC : rows, cols == DYNAMIC ? DYNAMIC : cols / 2, 2>;
    static constexpr int C0Size = 2;
    static_assert((cols == DYNAMIC) || (cols % C0Size == 0), "cols must be divisible by C0Size for Layout::MX_A_ND");

    PTO_INTERNAL TileShape2D() : Parent() {}
    PTO_INTERNAL TileShape2D(int dynamicRows, int dynamicCols) : Parent(1, 1, dynamicRows, dynamicCols / 2, 2) {}
    using Parent::Parent;
};

template <typename T, int rows, int cols>
struct BaseShape2D<T, rows, cols, Layout::MX_A_DN>
    : public Stride<GetBaseShape2DStride0<T, rows, cols>(), GetBaseShape2DStride0<T, rows, cols>(),
          rows == DYNAMIC ? DYNAMIC : rows * 2, 2, 1> {
    using Parent = Stride<GetBaseShape2DStride0<T, rows, cols>(), GetBaseShape2DStride0<T, rows, cols>(),
        rows == DYNAMIC ? DYNAMIC : rows * 2, 2, 1>;

    PTO_INTERNAL BaseShape2D() : Parent() {}
    PTO_INTERNAL BaseShape2D(int dynamicRows, int dynamicCols)
        : Parent(dynamicCols * dynamicRows, dynamicCols * dynamicRows, dynamicRows * 2, 2, 1)
    {
    }
    using Parent::Parent;
};

template <typename T, int rows, int cols>
struct TileShape2D<T, rows, cols, Layout::MX_A_DN>
    : public Shape<1, 1, cols == DYNAMIC ? DYNAMIC : cols / 2, rows == DYNAMIC ? DYNAMIC : rows, 2> {
    using Parent = Shape<1, 1, cols == DYNAMIC ? DYNAMIC : cols / 2, rows == DYNAMIC ? DYNAMIC : rows, 2>;
    static constexpr int C0Size = 2;
    static_assert((cols == DYNAMIC) || (cols % C0Size == 0), "cols must be divisible by C0Size for Layout::MX_A_DN");

    PTO_INTERNAL TileShape2D() : Parent() {}
    PTO_INTERNAL TileShape2D(int dynamicRows, int dynamicCols) : Parent(1, 1, dynamicCols / 2, dynamicRows, 2) {}
    using Parent::Parent;
};

template <typename T, int rows, int cols>
struct BaseShape2D<T, rows, cols, Layout::MX_B_NN>
    : public Stride<GetBaseShape2DStride0<T, rows, cols>(), (rows == DYNAMIC) ? DYNAMIC : rows * 16, 32, 2, 1> {
    static constexpr int FractalSize = 32;
    using Parent =
        Stride<GetBaseShape2DStride0<T, rows, cols>(), (rows == DYNAMIC) ? DYNAMIC : rows * 16, FractalSize, 2, 1>;

    PTO_INTERNAL BaseShape2D() : Parent() {}
    PTO_INTERNAL BaseShape2D(int dynamicRows, int dynamicCols)
        : Parent(dynamicCols * dynamicRows, dynamicRows * 16, FractalSize, 2, 1)
    {
    }
    using Parent::Parent;
};

template <typename T, int rows, int cols>
struct TileShape2D<T, rows, cols, Layout::MX_B_NN>
    : public Shape<1, cols == DYNAMIC ? DYNAMIC : cols / 16, rows == DYNAMIC ? DYNAMIC : rows / 2, 16, 2> {
    using Parent = Shape<1, cols == DYNAMIC ? DYNAMIC : cols / 16, rows == DYNAMIC ? DYNAMIC : rows / 2, 16, 2>;
    static constexpr int C0Size = 2;
    static_assert((rows == DYNAMIC) || (rows % C0Size == 0), "rows must be divisible by C0Size for Layout::MX_B_NN");

    PTO_INTERNAL TileShape2D() : Parent() {}
    PTO_INTERNAL TileShape2D(int dynamicRows, int dynamicCols) : Parent(1, dynamicCols / 16, dynamicRows / 2, 16, 2) {}
    using Parent::Parent;
};

template <typename T, int rows, int cols>
struct BaseShape2D<T, rows, cols, Layout::MX_B_ND>
    : public Stride<GetBaseShape2DStride0<T, rows, cols>(), GetBaseShape2DStride0<T, rows, cols>(),
          cols == DYNAMIC ? DYNAMIC : cols * 2, 2, 1> {
    using Parent = Stride<GetBaseShape2DStride0<T, rows, cols>(), GetBaseShape2DStride0<T, rows, cols>(),
        cols == DYNAMIC ? DYNAMIC : cols * 2, 2, 1>;

    PTO_INTERNAL BaseShape2D() : Parent() {}
    PTO_INTERNAL BaseShape2D(int dynamicRows, int dynamicCols)
        : Parent(dynamicCols * dynamicRows, dynamicCols * dynamicRows, dynamicCols * 2, 2, 1)
    {
    }
    using Parent::Parent;
};

template <typename T, int rows, int cols>
struct TileShape2D<T, rows, cols, Layout::MX_B_ND>
    : public Shape<1, 1, rows == DYNAMIC ? DYNAMIC : rows / 2, cols == DYNAMIC ? DYNAMIC : cols, 2> {
    using Parent = Shape<1, 1, rows == DYNAMIC ? DYNAMIC : rows / 2, cols == DYNAMIC ? DYNAMIC : cols, 2>;
    static constexpr int C0Size = 2;
    static_assert((rows == DYNAMIC) || (rows % C0Size == 0), "rows must be divisible by C0Size for Layout::MX_B_ND");

    PTO_INTERNAL TileShape2D() : Parent() {}
    PTO_INTERNAL TileShape2D(int dynamicRows, int dynamicCols) : Parent(1, 1, dynamicRows / 2, dynamicCols, 2) {}
    using Parent::Parent;
};

template <typename T, int rows, int cols>
struct BaseShape2D<T, rows, cols, Layout::MX_B_DN>
    : public Stride<GetBaseShape2DStride0<T, rows, cols>(), GetBaseShape2DStride0<T, rows, cols>(),
          rows == DYNAMIC ? DYNAMIC : rows, 2, 1> {
    using Parent = Stride<GetBaseShape2DStride0<T, rows, cols>(), GetBaseShape2DStride0<T, rows, cols>(),
        rows == DYNAMIC ? DYNAMIC : rows, 2, 1>;

    PTO_INTERNAL BaseShape2D() : Parent() {}
    PTO_INTERNAL BaseShape2D(int dynamicRows, int dynamicCols)
        : Parent(dynamicCols * dynamicRows, dynamicCols * dynamicRows, dynamicRows, 2, 1)
    {
    }
    using Parent::Parent;
};

template <typename T, int rows, int cols>
struct TileShape2D<T, rows, cols, Layout::MX_B_DN>
    : public Shape<1, 1, cols == DYNAMIC ? DYNAMIC : cols, rows == DYNAMIC ? DYNAMIC : rows / 2, 2> {
    using Parent = Shape<1, 1, cols == DYNAMIC ? DYNAMIC : cols, rows == DYNAMIC ? DYNAMIC : rows / 2, 2>;
    static constexpr int C0Size = 2;
    static_assert((rows == DYNAMIC) || (rows % C0Size == 0), "rows must be divisible by C0Size for Layout::MX_B_DN");

    PTO_INTERNAL TileShape2D() : Parent() {}
    PTO_INTERNAL TileShape2D(int dynamicRows, int dynamicCols) : Parent(1, 1, dynamicCols, dynamicRows / 2, 2) {}
    using Parent::Parent;
};

namespace TileConfig {
static constexpr int alignedSize = 32;
static constexpr int fixedRowSize = 16;
static constexpr int fixedColSize = 16;
static constexpr int fixedMxRowSize = 16;
static constexpr int fixedMxColSize = 2;
static constexpr int fractalABSize = 512;
static constexpr int fractalCSize = 1024;
static constexpr int fractalMxSize = 32;
static constexpr int cElemSize = 4;
} // namespace TileConfig

namespace ConvTileDetail {
    constexpr int MAX_CONVTILE_DIM = 6; // max support dim of convtile
    template<int N, int... Shapes>
    struct GetNthShape {
        static constexpr int value = []() {
            int idx = 0;
            int val = 0;
            ((idx == N ? (val = Shapes, idx++) : idx++), ...);
            return val;
        }();
    };

    template<int... Shapes>
    struct CountDynamicDim {
        static constexpr int value = []() {
            int count = 0;
            count += (GetNthShape<0, Shapes...>::value == DYNAMIC ? 1 : 0);
            count += (GetNthShape<1, Shapes...>::value == DYNAMIC ? 1 : 0);
            count += (GetNthShape<2, Shapes...>::value == DYNAMIC ? 1 : 0);
            count += (GetNthShape<3, Shapes...>::value == DYNAMIC ? 1 : 0);
            count += (GetNthShape<4, Shapes...>::value == DYNAMIC ? 1 : 0);
            count += (GetNthShape<5, Shapes...>::value == DYNAMIC ? 1 : 0);
            return count;
        }();
    };

    template<int DimIdx, int... Shapes>
    struct AssignDynamicDim {
        static void apply(int* shape, const int* vals, int& val_idx) {
            if constexpr (GetNthShape<DimIdx, Shapes...>::value == DYNAMIC) {
                shape[DimIdx] = vals[val_idx++];
            }
            if constexpr (DimIdx + 1 < static_cast<int>(MAX_CONVTILE_DIM)) {
                AssignDynamicDim<DimIdx + 1, Shapes...>::apply(shape, vals, val_idx);
            }
        }
    };
}

template<int... Shapes>
struct ConvTileShape {
    static constexpr int totalDimCount = sizeof...(Shapes);
    static constexpr int dynamicDimCount = ConvTileDetail::CountDynamicDim<Shapes...>::value;
    static constexpr int staticShape[static_cast<int>(ConvTileDetail::MAX_CONVTILE_DIM)] = {
        ConvTileDetail::GetNthShape<0, Shapes...>::value,
        ConvTileDetail::GetNthShape<1, Shapes...>::value,
        ConvTileDetail::GetNthShape<2, Shapes...>::value,
        ConvTileDetail::GetNthShape<3, Shapes...>::value,
        ConvTileDetail::GetNthShape<4, Shapes...>::value,
        ConvTileDetail::GetNthShape<5, Shapes...>::value
    };

    PTO_INTERNAL ConvTileShape(int n1, int n2, int n3, int n4, int n5, int n6) {
        if constexpr (staticShape[0] == DYNAMIC) shape[0] = n1;
        if constexpr (staticShape[1] == DYNAMIC) shape[1] = n2;
        if constexpr (staticShape[2] == DYNAMIC) shape[2] = n3;
        if constexpr (staticShape[3] == DYNAMIC) shape[3] = n4;
        if constexpr (staticShape[4] == DYNAMIC) shape[4] = n5;
        if constexpr (staticShape[5] == DYNAMIC) shape[5] = n6;
    }

    PTO_INTERNAL ConvTileShape() {
        if constexpr (staticShape[0] == DYNAMIC) shape[0] = 1;
        if constexpr (staticShape[1] == DYNAMIC) shape[1] = 1;
        if constexpr (staticShape[2] == DYNAMIC) shape[2] = 1;
        if constexpr (staticShape[3] == DYNAMIC) shape[3] = 1;
        if constexpr (staticShape[4] == DYNAMIC) shape[4] = 1;
        if constexpr (staticShape[5] == DYNAMIC) shape[5] = 1;
    }

    PTO_INTERNAL ConvTileShape(int n) {
        static_assert(dynamicDimCount == 1,
            "1-parameter constructor is only applicable to Shape with 1 dynamic dimension.");
        
        int val_idx = 0;
        const int vals[] = {n};
        ConvTileDetail::AssignDynamicDim<0, Shapes...>::apply(shape, vals, val_idx);
    }

    PTO_INTERNAL ConvTileShape(int n1, int n2) {
        static_assert(dynamicDimCount == 2,
            "2-parameter constructor is only applicable to Shape with 2 dynamic dimensions.");

        int val_idx = 0;
        const int vals[] = {n1, n2};
        ConvTileDetail::AssignDynamicDim<0, Shapes...>::apply(shape, vals, val_idx);
    }

    PTO_INTERNAL ConvTileShape(int n1, int n2, int n3) {
        static_assert(dynamicDimCount == 3,
            "3-parameter constructor is only applicable to Shape with 3 dynamic dimensions.");

        int val_idx = 0;
        const int vals[] = {n1, n2, n3};
        ConvTileDetail::AssignDynamicDim<0, Shapes...>::apply(shape, vals, val_idx);
    }

    PTO_INTERNAL ConvTileShape(int n1, int n2, int n3, int n4) {
        static_assert(dynamicDimCount == 4,
            "4-parameter constructor is only applicable to Shape with 4 dynamic dimensions.");

        int val_idx = 0;
        const int vals[] = {n1, n2, n3, n4};
        ConvTileDetail::AssignDynamicDim<0, Shapes...>::apply(shape, vals, val_idx);
    }
    PTO_INTERNAL ConvTileShape(int n1, int n2, int n3, int n4, int n5) {
        static_assert(dynamicDimCount == 5,
            "5-parameter constructor is only applicable to Shape with 5 dynamic dimensions.");

        int val_idx = 0;
        const int vals[] = {n1, n2, n3, n4, n5};
        ConvTileDetail::AssignDynamicDim<0, Shapes...>::apply(shape, vals, val_idx);
    }

public:
    int shape[static_cast<int>(ConvTileDetail::MAX_CONVTILE_DIM)] = {1};
};

template <TileType Loc_, typename Element_, const int BufferSize_, Layout Layout_, typename Shape_>
struct ConvTile {
public:
    using DType = Element_;
    using ShapeType = Shape_;
    static constexpr TileType Loc = Loc_;
    static constexpr int bufferSize = BufferSize_;
    static constexpr Layout layout = Layout_;

    static constexpr int totalDimCount = ShapeType::totalDimCount;
    static_assert(totalDimCount >= 1 && totalDimCount <= ConvTileDetail::MAX_CONVTILE_DIM,
        "ConvTile only support 1D~6D Shapes!");
    static constexpr int staticShape[ConvTileDetail::MAX_CONVTILE_DIM] = {ShapeType::staticShape[0],
        ShapeType::staticShape[1], ShapeType::staticShape[2], ShapeType::staticShape[3], ShapeType::staticShape[4],
        ShapeType::staticShape[5]};
    static constexpr int dynamicDimCount = ShapeType::dynamicDimCount;
    static constexpr bool isDynamicDim[ConvTileDetail::MAX_CONVTILE_DIM] = {ShapeType::staticShape[0] == DYNAMIC,
        ShapeType::staticShape[1] == DYNAMIC, ShapeType::staticShape[2] == DYNAMIC,
        ShapeType::staticShape[3] == DYNAMIC, ShapeType::staticShape[4] == DYNAMIC,
        ShapeType::staticShape[5] == DYNAMIC};
    int shape[ConvTileDetail::MAX_CONVTILE_DIM] = {1};

    PTO_INTERNAL constexpr int GetShape(int dim) const {
        if (dim < 0 || dim >= totalDimCount) {
            return -1;
        }
        return isDynamicDim[dim] ? shape[dim] : staticShape[dim];
    }

    PTO_INTERNAL ConvTile() = default;

    template <typename... Ints>
    PTO_INTERNAL void SetDynamicShape(Ints... vals) {
        static_assert(
            sizeof...(vals) == dynamicDimCount, "Number of dynamic values does not match dynamic dimension count!");
        static_assert((std::is_same_v<Ints, int> && ...), "Dynamic values must be int type!");

        int idx = 0;
        const int dynamicVals[] = {vals...};
        for (int i = 0; i < ConvTileDetail::MAX_CONVTILE_DIM; ++i) {
            if (isDynamicDim[i]) {
                shape[i] = dynamicVals[idx++];
            }
        }
    }

    template <typename... Ints>
    PTO_INTERNAL explicit ConvTile(Ints... dynamicVals) {
        SetDynamicShape(dynamicVals...);
    }

#ifdef __PTO_AUTO__
    using TileDType = typename MemoryQualifier<Loc_, DType>::type tileSize(bufferSize);
#else
    using TileDType = typename MemoryQualifier<Loc_, DType>::type;
#endif

    AICORE TileDType &data() {
        return data_;
    }
    AICORE const TileDType &data() const {
        return data_;
    }
    template <typename T, typename AddrType>
    friend AICORE void TASSIGN_IMPL(T &tile, AddrType addr);

private:
    AICORE void assignData(TileDType data) {
        data_ = data;
    }
    TileDType data_;
};

template <TileType Loc_, typename Element_, const int Rows_, const int Cols_,
    const BLayout BFractal_ = BLayout::RowMajor, const int RowValid_ = Rows_, const int ColValid_ = Cols_,
    const SLayout SFractal_ = SLayout::NoneBox, const int SFractalSize_ = TileConfig::fractalABSize,
    const PadValue PadVal_ = PadValue::Null, const CompactMode Compact_ = CompactMode::Null>
struct Tile {
  public:
    using DType = Element_;

    static constexpr int getInnerRow() {
      if constexpr (SFractalSize_ == TileConfig::fractalCSize) {
        static_assert(sizeof(DType) == TileConfig::cElemSize,
                      "Size of datatype != 4");
        return TileConfig::fixedRowSize;
      } else if constexpr (SFractalSize_ == TileConfig::fractalMxSize) {
        return TileConfig::fixedMxRowSize;
      } else {
        return isBoxedLayout
                   ? (isInnerRowMajor ? TileConfig::fixedRowSize
                                      : TileConfig::alignedSize / sizeof(DType))
                   : 1;
      }
    }

    static constexpr int getInnerCol() {
      if constexpr (SFractalSize_ == TileConfig::fractalCSize) {
        static_assert(sizeof(DType) == TileConfig::cElemSize,
                      "Size of datatype != 4");
        return TileConfig::fixedColSize;
      } else if constexpr (SFractalSize_ == TileConfig::fractalMxSize) {
        return TileConfig::fixedMxColSize;
      } else {
        return isBoxedLayout
                   ? (isInnerRowMajor ? TileConfig::alignedSize / sizeof(DType)
                                      : TileConfig::fixedColSize)
                   : 1;
      }
    }

    static constexpr TileType Loc = Loc_;
    static constexpr int Rows = Rows_;
    static constexpr int Cols = Cols_;
    static constexpr int RowStride = BFractal_ == BLayout::RowMajor ? Cols : 1;
    static constexpr int ColStride = BFractal_ == BLayout::RowMajor ? 1 : Rows;

    static constexpr int ValidRow = RowValid_;
    static constexpr int ValidCol = ColValid_;
    static_assert(Rows > 0 && ValidRow <= Rows && Cols > 0 && ValidCol <= Cols,
                  "Invalid Tile Layout.");

    static constexpr BLayout BFractal = BFractal_;
    static constexpr SLayout SFractal = SFractal_;
    static constexpr int Numel = Rows * Cols;
    static constexpr bool isRowMajor = BFractal_ == BLayout::RowMajor;

    static constexpr int SFractalSize = SFractalSize_;
    static constexpr PadValue PadVal = PadVal_;
    static constexpr CompactMode Compact = Compact_;

    __tf__ AICORE void SetValue(const uint32_t offset, const DType val) {
        static_assert(Loc == TileType::Vec, "Location of tile must be Location::Vec.");
        __ubuf__ DType *ptr = (__ubuf__ DType *)__cce_get_tile_ptr(data_);
        *(ptr + offset) = val;
    }

    __tf__ AICORE DType GetValue(const uint32_t offset) {
        static_assert(Loc == TileType::Vec, "Location of tile must be Location::Vec.");
        __ubuf__ DType *ptr = (__ubuf__ DType *)__cce_get_tile_ptr(data_);
        return *(ptr + offset);
    }
    // constructor for static shape
    AICORE Tile() {};

    // constructor for both dimensions are runtime variables
    template <int RowMask = ValidRow, int ColMask = ValidCol>
    AICORE
    Tile(std::enable_if_t<RowMask == DYNAMIC && ColMask == DYNAMIC, size_t> VR,
         std::enable_if_t<RowMask == DYNAMIC && ColMask == DYNAMIC, size_t> VC) {
      RowMaskInternal = VR;
      ColMaskInternal = VC;
    }

    // constructor for row dimension is runtime variables
    template <int RowMask = ValidRow, int ColMask = ValidCol>
    AICORE
    Tile(std::enable_if_t<(RowMask == DYNAMIC) && (ColMask > 0), size_t> VR) {
      RowMaskInternal = VR;
    }

    // constructor for col dimension is runtime variables
    template <int RowMask = ValidRow, int ColMask = ValidCol>
    AICORE
    Tile(std::enable_if_t<(RowMask > 0) && (ColMask == DYNAMIC), size_t> VC) {
      ColMaskInternal = VC;
    }

    static constexpr bool isBoxedLayout = (SFractal != SLayout::NoneBox);
    static constexpr bool isInnerRowMajor = (SFractal == SLayout::RowMajor);
    static constexpr bool isInnerColMajor = (SFractal == SLayout::ColMajor);

    static constexpr int InnerRows = getInnerRow();
    static constexpr int InnerCols = getInnerCol();

    static constexpr int InnerNumel = InnerRows * InnerCols;

    static_assert(InnerRows != 0 && InnerCols != 0,
                  "rows or cols of fractal size is 0.");
    static_assert((Loc == TileType::Vec) || (SFractalSize_ == TileConfig::fractalMxSize) || (Rows % InnerRows == 0),
                  "Layout rows must be divisible by inner box rows");
    static_assert(Cols % InnerCols == 0,
                  "Layout cols must be divisible by inner box cols");

    static_assert(
        (BFractal_ == BLayout::RowMajor && SFractal_ == SLayout::NoneBox && Cols * sizeof(DType) % TileConfig::alignedSize == 0) ||
        (BFractal_ == BLayout::ColMajor && SFractal_ == SLayout::NoneBox && Rows * sizeof(DType) % TileConfig::alignedSize == 0) ||
        (SFractal_ != SLayout::NoneBox) &&
        (((Loc == TileType::Vec) || (SFractalSize_ == TileConfig::fractalMxSize) || (Rows % InnerRows == 0)) && Cols % InnerCols == 0),
        "BFractal_ is RowMajor and SFractal_ is NoneBox: Rows must be 32 bytes align, \
         BFractal_ is ColMajor and SFractal_ is NoneBox: Cols must be 32 bytes align, \
         SFractal_ in not NoneBox: Rows/Cols must be integer multiple of InnerRows/InnerCols."
         );

    static_assert(SFractalSize_ == TileConfig::fractalABSize ||
                      SFractalSize_ == TileConfig::fractalCSize ||
                      SFractalSize_ == TileConfig::fractalMxSize,
                  "SFractalSize_ illegal");

#ifdef __CPU_SIM
    using TileDType = Tile::DType[Rows*Cols];
#else
    #ifdef __PTO_AUTO__
        using TileDType = typename MemoryQualifier<Loc, DType>::type tile_size(Rows * Cols);
    #else
        using TileDType = typename MemoryQualifier<Loc, DType>::type;
    #endif
#endif

    AICORE TileDType &data() { return data_; }
    AICORE const TileDType &data() const { return data_; }

    int RowMaskInternal;
    int ColMaskInternal;

    template <int RowMask = ValidRow>
    AICORE static constexpr std::enable_if_t<(RowMask > 0), int> GetValidRow() {
        return RowMask;
    }

    template <int RowMask = ValidRow>
    AICORE std::enable_if_t<RowMask == DYNAMIC, int> GetValidRow() const {
        return RowMaskInternal;
    }

    template <int ColMask = ValidCol>
    AICORE static constexpr std::enable_if_t<(ColMask > 0), int> GetValidCol() {
        return ColMask;
    }

    template <int ColMask = ValidCol>
    AICORE std::enable_if_t<ColMask == DYNAMIC, int> GetValidCol() const {
        return ColMaskInternal;
    }

    template <typename T, typename AddrType>
    friend AICORE void TASSIGN_IMPL(T &tile, AddrType addr);

    PTO_INTERNAL bool GetKAligned() const {
        return isKAligned_;
    }
    PTO_INTERNAL void SetKAligned(bool isKAligned) {
        isKAligned_ = isKAligned;
    }
  private:
    AICORE void assignData(TileDType data) { data_ = data; }
    TileDType data_;
    bool isKAligned_; // K-Alignedment for A3
};

#ifdef MEMORY_BASE
template <typename Element_, const int Rows_, const int Cols_,
          const int RowValid_ = Rows_, const int ColValid_ = Cols_>
using TileLeft =
    Tile<TileType::Left, Element_, Rows_, Cols_, BLayout::RowMajor, RowValid_,
         ColValid_, SLayout::RowMajor, TileConfig::fractalABSize>;

template <typename Element_, const int Rows_, const int Cols_,
          const int RowValid_ = Rows_, const int ColValid_ = Cols_>
using TileLeftCompact =
    Tile<TileType::Left, Element_, Rows_, Cols_, BLayout::RowMajor, RowValid_,
         ColValid_, SLayout::RowMajor, TileConfig::fractalABSize, PadValue::Null, CompactMode::Normal>;
#endif

#if defined (REGISTER_BASE) || defined (__CPU_SIM)
template <typename Element_, const int Rows_, const int Cols_,
          const int RowValid_ = Rows_, const int ColValid_ = Cols_>
using TileLeft =
    Tile<TileType::Left, Element_, Rows_, Cols_, BLayout::ColMajor, RowValid_,
         ColValid_, SLayout::RowMajor, TileConfig::fractalABSize>;

template <typename Element_, const int Rows_, const int Cols_,
          const int RowValid_ = Rows_, const int ColValid_ = Cols_>
using TileLeftCompact =
    Tile<TileType::Left, Element_, Rows_, Cols_, BLayout::ColMajor, RowValid_,
         ColValid_, SLayout::RowMajor, TileConfig::fractalABSize, PadValue::Null, CompactMode::Normal>;
#endif

template <typename Element_, const int Rows_, const int Cols_,
          const int RowValid_ = Rows_, const int ColValid_ = Cols_>
using TileRight =
  Tile<TileType::Right, Element_, Rows_, Cols_, BLayout::RowMajor, RowValid_,
       ColValid_, SLayout::ColMajor, TileConfig::fractalABSize>;

template <typename Element_, const int Rows_, const int Cols_,
          const int RowValid_ = Rows_, const int ColValid_ = Cols_>
using TileRightCompact =
  Tile<TileType::Right, Element_, Rows_, Cols_, BLayout::RowMajor, RowValid_,
       ColValid_, SLayout::ColMajor, TileConfig::fractalABSize, PadValue::Null, CompactMode::Normal>;

template <typename Element_, const int Rows_, const int Cols_,
          const int RowValid_ = Rows_, const int ColValid_ = Cols_>
using TileLeftScale =
  Tile<TileType::ScaleLeft, Element_, Rows_, Cols_, BLayout::RowMajor, RowValid_,
       ColValid_, SLayout::RowMajor, TileConfig::fractalMxSize>;

template <typename Element_, const int Rows_, const int Cols_,
          const int RowValid_ = Rows_, const int ColValid_ = Cols_>
using TileLeftScaleCompact =
  Tile<TileType::ScaleLeft, Element_, Rows_, Cols_, BLayout::RowMajor, RowValid_,
       ColValid_, SLayout::RowMajor, TileConfig::fractalMxSize, PadValue::Null, CompactMode::Normal>;

template <typename Element_, const int Rows_, const int Cols_,
          const int RowValid_ = Rows_, const int ColValid_ = Cols_>
using TileRightScale =
  Tile<TileType::ScaleRight, Element_, Rows_, Cols_, BLayout::ColMajor, RowValid_,
       ColValid_, SLayout::ColMajor, TileConfig::fractalMxSize>;

template <typename Element_, const int Rows_, const int Cols_,
          const int RowValid_ = Rows_, const int ColValid_ = Cols_>
using TileRightScaleCompact =
  Tile<TileType::ScaleRight, Element_, Rows_, Cols_, BLayout::ColMajor, RowValid_,
       ColValid_, SLayout::ColMajor, TileConfig::fractalMxSize, PadValue::Null, CompactMode::Normal>;

template <typename Element_, const int Rows_, const int Cols_,
          const int RowValid_ = Rows_, const int ColValid_ = Cols_>
using TileAcc =
    Tile<TileType::Acc, Element_, Rows_, Cols_, BLayout::ColMajor, RowValid_,
         ColValid_, SLayout::RowMajor, TileConfig::fractalCSize>;

template <typename Element_, const int Rows_, const int Cols_,
          const int RowValid_ = Rows_, const int ColValid_ = Cols_>
using TileAccCompact =
    Tile<TileType::Acc, Element_, Rows_, Cols_, BLayout::ColMajor, RowValid_,
         ColValid_, SLayout::RowMajor, TileConfig::fractalCSize, PadValue::Null, CompactMode::Normal>;

template <typename T> struct is_global : std::false_type {};
template <typename T> struct is_tile : std::false_type {
    static constexpr SLayout layout_enum = SLayout::NoneBox;
};

template <typename Element_, typename Shape_, typename Stride_, Layout Layout_>
struct is_global<GlobalTensor<Element_, Shape_, Stride_, Layout_>>
    : std::true_type {};

template <TileType Loc_, typename Element_, const int Rows_, const int Cols_, const BLayout BFractal_,
    const int RowValid_, const int ColValid_, const SLayout SFractal_, const int SFractalSize_, const PadValue PadVal_,
    const CompactMode Compact_>
struct is_tile<
    Tile<Loc_, Element_, Rows_, Cols_, BFractal_, RowValid_, ColValid_, SFractal_, SFractalSize_, PadVal_, Compact_>>
    : std::true_type {
    static constexpr SLayout layout_enum = SFractal_;
};

template <typename T>
constexpr bool is_boxed_tile =
    is_tile<T>::value && (is_tile<T>::layout_enum != SLayout::NoneBox);

template<typename T>
struct is_conv_tile : std::false_type {};
template<TileType Loc_, typename Element_, const int BufferSize_, Layout Layout_, typename Shape_>
struct is_conv_tile<ConvTile<Loc_, Element_, BufferSize_, Layout_, Shape_>> : std::true_type {};

template <typename tile_shape> struct is_Nz_layout {
  static constexpr bool value = !tile_shape::isRowMajor &&
                                tile_shape::isBoxedLayout &&
                                tile_shape::isInnerRowMajor;
};

template <typename tile_shape> struct is_Zn_layout {
  static constexpr bool value = tile_shape::isRowMajor &&
                                tile_shape::isBoxedLayout &&
                                tile_shape::isInnerColMajor;
};

template <typename tile_shape> struct is_Zz_layout {
  static constexpr bool value = tile_shape::isRowMajor &&
                                tile_shape::isBoxedLayout &&
                                tile_shape::isInnerRowMajor;
};
template <typename T> constexpr bool is_conv_tile_v = is_conv_tile<T>::value;

template <typename T> constexpr bool is_global_data_v = is_global<T>::value;

template <typename T> constexpr bool is_tile_data_v = is_tile<T>::value;

template <typename T> constexpr bool is_boxed_data_v = is_boxed_tile<T>;

// Get the memory offset of a tile element from logical coordinates
template <typename TileT> PTO_INTERNAL size_t GetTileOffset(int row, int col) {
  static_assert(is_tile_data_v<TileT>, "tile_offset only accepts Tile types.");
  if constexpr (!TileT::isBoxedLayout) {
    return row * TileT::RowStride + col * TileT::ColStride;
  } else {
    // Compute block coordinates
    int BlockRow = row / TileT::InnerRows;
    int BlockCol = col / TileT::InnerCols;
    // Compute intra-block offset
    int InnerRow = row % TileT::InnerRows;
    int InnerCol = col % TileT::InnerCols;
    // Compute block numbers
    static constexpr int BlockNumRow = TileT::Rows / TileT::InnerRows;
    static constexpr int BlockNumCol = TileT::Cols / TileT::InnerCols;
    if constexpr (is_Nz_layout<TileT>::value) {
      return (BlockNumRow * BlockCol + BlockRow) * TileT::InnerNumel +
             InnerRow * TileT::InnerCols + InnerCol;
    } else if constexpr (is_Zn_layout<TileT>::value) {
      return (BlockNumCol * BlockRow + BlockCol) * TileT::InnerNumel +
             InnerCol * TileT::InnerRows + InnerRow;
    } else if constexpr (is_Zz_layout<TileT>::value) {
      return (BlockNumCol * BlockRow + BlockCol) * TileT::InnerNumel +
             InnerRow * TileT::InnerCols + InnerCol;
    } else {
      // This branch should not be instantiated.
      static_assert(sizeof(TileT) == 0,
                    "Unsupported layout in Tile, fractal tiles should be "
                    "Nz or Zn layout.");
    }
  }
}

} // namespace pto

#endif
