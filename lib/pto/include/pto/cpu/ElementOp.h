/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef ELEMENT_OP_HPP
#define ELEMENT_OP_HPP

#include <cmath>

#include "pto/common/pto_tile.hpp"
#include "pto/cpu/tile_offsets.hpp"

namespace pto {
    enum class ElementOp {
        // binary operation
        OP_ADD = 0,
        OP_SUB,
        OP_MUL,
        OP_DIV,
        OP_REM,
        OP_SHL,
        OP_SHR,
        OP_AND,
        OP_OR,
        OP_XOR,
        OP_MAX,
        OP_MIN,
        OP_CMP, // compare mode need extra parameters
        OP_PRELU,
        // unary operation
        OP_EXP,
        OP_ABS,
        OP_LOG,
        OP_NEG,
        OP_NOT,
        OP_SQRT,
        OP_RECIP,
        OP_RSQRT,
        OP_RELU,

        // ternary operation
        OP_SEL,
        OP_ADDC,
        OP_SUBC,

        // Tile-Scalar Operation
        // Input scala
        OP_EXPANDS,
        // Input tile and scala
        OP_ADDS,
        OP_SUBS,
        OP_MULS,
        OP_DIVS,
        OP_REMS,
        OP_MAXS,
        OP_MINS,
        OP_ANDS,
        OP_ORS,
        OP_XORS,
        OP_CMPS,
        OP_LRELU,
        // Input tile0 tile1 and scala
        OP_SELS,
        OP_ADDCS,
        OP_SUBCS,
    };

    template<typename DType, ElementOp op>
    struct ElementOpCal {
        static void apply(DType &dst, DType &src0, DType &src1, size_t) {
            static_assert(false, "Unsupport element op.");
        }
    };

    template<typename DType>
    struct ElementOpCal<DType, ElementOp::OP_ADD> {
        static void apply(DType &dst, DType &src0, DType &src1, size_t) {
            dst = src0 + src1;
        }
    };

    template<typename DType>
    struct ElementOpCal<DType, ElementOp::OP_SUB> {
        static void apply(DType &dst, DType &src0, DType &src1, size_t) {
            dst = src0 - src1;
        }
    };

    template<typename DType>
    struct ElementOpCal<DType, ElementOp::OP_MUL> {
        static void apply(DType &dst, DType &src0, DType &src1, size_t) {
            dst = src0 * src1;
        }
    };

    template<typename DType>
    struct ElementOpCal<DType, ElementOp::OP_DIV> {
        static void apply(DType &dst, DType &src0, DType &src1, size_t) {
            if (src1 != static_cast<DType>(0)) {
                dst = src0 / src1;
            } else {
                PTO_ASSERT(false, "illegal src is zero");
            }
        }
    };

    template<typename DType>
    struct ElementOpCal<DType, ElementOp::OP_REM> {
        static void apply(DType &dst, DType &src0, DType &src1, size_t) {
            if (src1 != static_cast<DType>(0)) {
                if constexpr (std::is_integral_v<DType>) {
                    dst = src0 % src1;
                } else {
                    dst = static_cast<DType>(std::fmod(static_cast<double>(src0),
                                                       static_cast<double>(src1)));
                }
            } else {
                PTO_ASSERT(false, "illegal src is zero");
            }
        }
    };

    template<typename DType>
    struct ElementOpCal<DType, ElementOp::OP_SHL> {
        static void apply(DType &dst, DType &src0, DType &src1, size_t) {
            dst = src0 << src1;
        }
    };

    template<typename DType>
    struct ElementOpCal<DType, ElementOp::OP_SHR> {
        static void apply(DType &dst, DType &src0, DType &src1, size_t) {
            dst = src0 >> src1;
        }
    };

    template<typename DType>
    struct ElementOpCal<DType, ElementOp::OP_AND> {
        static void apply(DType &dst, DType &src0, DType &src1, size_t) {
            dst = src0 & src1;
        }
    };
    
    template<typename DType>
    struct ElementOpCal<DType, ElementOp::OP_OR> {
        static void apply(DType &dst, DType &src0, DType &src1, size_t) {
            dst = src0 | src1;
        }
    };

    template<typename DType>
    struct ElementOpCal<DType, ElementOp::OP_XOR> {
        static void apply(DType &dst, DType &src0, DType &src1, size_t) {
            dst = src0 ^ src1;
        }
    };

    template<typename DType>
    struct ElementOpCal<DType, ElementOp::OP_MAX> {
        static void apply(DType &dst, DType &src0, DType &src1, size_t) {
            dst = std::max(src0, src1);
        }
    };

    template<typename DType>
    struct ElementOpCal<DType, ElementOp::OP_MIN> {
        static void apply(DType &dst, DType &src0, DType &src1, size_t) {
            dst = std::min(src0, src1);
        }
    };

    template<typename DType>
    struct ElementOpCal<DType, ElementOp::OP_CMP> {
        static void apply(DType &dst, DType &src0, DType &src1, size_t extra = 0) {
            switch (static_cast<CmpMode>(extra)) {
                case CmpMode::EQ:
                    dst = (src0 == src1);
                    break;
                case CmpMode::NE:
                    dst = (src0 != src1);
                    break;
                case CmpMode::GT:
                    dst = (src0 > src1);
                    break;
                case CmpMode::LT:
                    dst = (src0 < src1);
                    break;
                case CmpMode::GE:
                    dst = (src0 >= src1);
                    break;
                case CmpMode::LE:
                    dst = (src0 <= src1);
                    break;
                default:
                    dst = (src0 == src1);
                    break;
            }
        }
    };

    template<typename DType>
    struct ElementOpCal<DType, ElementOp::OP_PRELU> {
        static void apply(DType &dst, DType &src0, DType &src1, size_t) {
            dst = ((src0 > static_cast<DType>(0)) ? src0 : (src0 * src1));
        }
    };

    template<typename DType>
    struct ElementOpCal<DType, ElementOp::OP_EXP> {
        static void apply(DType &dst, DType &src) {
            dst = static_cast<DType>(std::exp(static_cast<double>(src)));
        }
    };

    template<typename DType>
    struct ElementOpCal<DType, ElementOp::OP_ABS> {
        static void apply(DType &dst, DType &src) {
            dst = static_cast<DType>(std::abs(static_cast<double>(src)));
        }
    };

    template<typename DType>
    struct ElementOpCal<DType, ElementOp::OP_LOG> {
        static void apply(DType &dst, DType &src) {
            dst = static_cast<DType>(std::log(static_cast<double>(src)));
        }
    };

    template<typename DType>
    struct ElementOpCal<DType, ElementOp::OP_NEG> {
        static void apply(DType &dst, DType &src) {
            dst = -src;
        }
    };

    template<typename DType>
    struct ElementOpCal<DType, ElementOp::OP_NOT> {
        static void apply(DType &dst, DType &src) {
            dst = ~src;
        }
    };

    template<typename DType>
    struct ElementOpCal<DType, ElementOp::OP_SQRT> {
        static void apply(DType &dst, DType &src) {
            dst = static_cast<DType>(std::sqrt(static_cast<double>(src)));
        }
    };

    template<typename DType>
    struct ElementOpCal<DType, ElementOp::OP_RECIP> {
        static void apply(DType &dst, DType &src) {
            if (src != static_cast<DType>(0)) {
                dst = 1 / src;
            } else {
                PTO_ASSERT(false, "illegal src is zero");
            }
        }
    };

    template<typename DType>
    struct ElementOpCal<DType, ElementOp::OP_RSQRT> {
        static void apply(DType &dst, DType &src) {
            if (src != static_cast<DType>(0)) {
                dst = static_cast<DType>(1.0 / std::sqrt(static_cast<double>(src)));
            } else {
                PTO_ASSERT(false, "illegal src is zero");
            }
        }
    };

    template<typename DType>
    struct ElementOpCal<DType, ElementOp::OP_RELU> {
        static void apply(DType &dst, DType &src) {
            dst = std::max(src, static_cast<DType>(0));
        }
    };

    template<typename DType>
    struct ElementOpCal<DType, ElementOp::OP_SEL> {
        static void apply(DType &dst, DType &mask, DType &src0, DType &src1) {
            if (mask == 1) {
                dst = src0;
            } else {
                dst = src1;
            }
        }
    };

    template<typename DType>
    struct ElementOpCal<DType, ElementOp::OP_ADDC> {
        static void apply(DType &dst, DType &src0, DType &src1, DType &src2) {
            dst = src0 + src1 + src2;
        }
    };

    template<typename DType>
    struct ElementOpCal<DType, ElementOp::OP_SUBC> {
        static void apply(DType &dst, DType &src0, DType &src1, DType &src2) {
            dst = src0 - src1 + src2;
        }
    };

    template<typename DType>
    struct ElementOpCal<DType, ElementOp::OP_EXPANDS> {
        static void apply(DType &dst, DType &scalar) {
            dst = scalar;
        }
    };

    template<typename DType>
    struct ElementOpCal<DType, ElementOp::OP_ADDS> {
        static void apply(DType &dst, DType &src, DType &scalar, size_t) {
            dst = src + scalar;
        }
    };

    template<typename DType>
    struct ElementOpCal<DType, ElementOp::OP_SUBS> {
        static void apply(DType &dst, DType &src, DType &scalar, size_t) {
            dst = src - scalar;
        }
    };

    template<typename DType>
    struct ElementOpCal<DType, ElementOp::OP_MULS> {
        static void apply(DType &dst, DType &src, DType &scalar, size_t) {
            dst = src * scalar;
        }
    };

    template<typename DType>
    struct ElementOpCal<DType, ElementOp::OP_DIVS> {
        static void apply(DType &dst, DType &src, DType &scalar, size_t) {
            if (scalar != static_cast<DType>(0)) {
                dst = src / scalar;
            } else {
                PTO_ASSERT(false, "illegal src is zero");
            }
        }
    };

    template<typename DType>
    struct ElementOpCal<DType, ElementOp::OP_REMS> {
        static void apply(DType &dst, DType &src, DType &scalar, size_t) {
            if (scalar != static_cast<DType>(0)) {
                if constexpr (std::is_integral_v<DType>) {
                    dst = src % scalar;
                } else {
                    dst = static_cast<DType>(std::fmod(static_cast<double>(src),
                                                       static_cast<double>(scalar)));
                }
            } else {
                PTO_ASSERT(false, "illegal src is zero");
            }
        }
    };

    template<typename DType>
    struct ElementOpCal<DType, ElementOp::OP_MAXS> {
        static void apply(DType &dst, DType &src, DType &scalar, size_t) {
            dst = std::max(src, scalar);
        }
    };

    template<typename DType>
    struct ElementOpCal<DType, ElementOp::OP_ANDS> {
        static void apply(DType &dst, DType &src, DType &scalar, size_t) {
            dst = src & scalar;
        }
    };

    template<typename DType>
    struct ElementOpCal<DType, ElementOp::OP_ORS> {
        static void apply(DType &dst, DType &src, DType &scalar, size_t) {
            dst = src | scalar;
        }
    };

    template<typename DType>
    struct ElementOpCal<DType, ElementOp::OP_XORS> {
        static void apply(DType &dst, DType &src, DType &scalar, size_t) {
            dst = src ^ scalar;
        }
    };

    template<typename DType>
    struct ElementOpCal<DType, ElementOp::OP_CMPS> {
        static void apply(DType &dst, DType &src, DType &scalar, size_t extra) {
            switch (static_cast<CmpMode>(extra)) {
                case CmpMode::EQ:
                    dst = (src == scalar);
                    break;
                case CmpMode::NE:
                    dst = (src != scalar);
                    break;
                case CmpMode::GT:
                    dst = (src > scalar);
                    break;
                case CmpMode::LT:
                    dst = (src < scalar);
                    break;
                case CmpMode::GE:
                    dst = (src >= scalar);
                    break;
                case CmpMode::LE:
                    dst = (src <= scalar);
                    break;
                default:
                    static_assert(false, "Unsupport CMP_MODE.");
                    break;
            }
        }
    };

    template<typename DType>
    struct ElementOpCal<DType, ElementOp::OP_LRELU> {
        static void apply(DType &dst, DType &src, DType &scalar, size_t) {
            dst = (src > static_cast<DType>(0)) ? src : (src * scalar);
        }
    };

    template<typename DType>
    struct ElementOpCal<DType, ElementOp::OP_SELS> {
        static void apply(DType &dst, DType &scalar, DType &src0, DType &src1) {
            if (scalar == static_cast<DType>(1)) {
                dst = src0;
            } else {
                dst = src1;
            }
        }
    };

    template<typename DType>
    struct ElementOpCal<DType, ElementOp::OP_ADDCS> {
        static void apply(DType &dst, DType &src0, DType &scalar, DType &src1) {
            dst = src0 + scalar + src1;
        }
    };

    template<typename DType>
    struct ElementOpCal<DType, ElementOp::OP_SUBCS> {
        static void apply(DType &dst, DType &src0, DType &scalar, DType &src1) {
            dst = src0 - scalar + src1;
        }
    };
}
#endif
