/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef PTO__DATATYPE_IMPL_H
#define PTO__DATATYPE_IMPL_H

namespace pto{
template <typename T> struct TypeGet;
#if defined(__DAV_VEC__)
template <> struct TypeGet<bfloat16_t> {
    using T = vector_bf16;
};
#endif
template <> struct TypeGet<uint64_t> {
    using T = vector_u64;
};
template <> struct TypeGet<int64_t> {
    using T = vector_s64;
};
template <> struct TypeGet<uint32_t> {
    using T = vector_u32;
};
template <> struct TypeGet<int32_t> {
    using T = vector_s32;
};
template <> struct TypeGet<float> {
    using T = vector_f32;
};
template <> struct TypeGet<uint16_t> {
    using T = vector_u16;
};
template <> struct TypeGet<half> {
    using T = vector_f16;
};
template <> struct TypeGet<int16_t> {
    using T = vector_s16;
};
template <> struct TypeGet<uint8_t> {
    using T = vector_u8;
};
template <> struct TypeGet<int8_t> {
    using T = vector_s8;
};
} // namespace pto
#endif 