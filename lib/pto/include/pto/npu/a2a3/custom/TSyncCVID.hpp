/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TSYNC_CVID_HPP
#define TSYNC_CVID_HPP

#include <pto/common/type.hpp>
#include <pto/common/utils.hpp>

namespace pto {

// System reserved FFTS event ids 12-15 for CV comm (control + reserved)
enum CVCommFftsEvent : uint16_t {
    CV_COMM_CTRL = 12,
    CV_COMM_RSVD_13,
    CV_COMM_RSVD_14,
    CV_COMM_RSVD_15,
};

// Global CV comm defaults
constexpr int kCvCommSlotBytes = 512;
constexpr int kCvMaxCores = 25;

enum CVSyncMode : uint16_t {
    C_ALL_CORE_SYNC = 0,
    V_ALL_CORE_SYNC = 0,
    V_SUBCORES_SYNC = 1,
    CV_CORE_SYNC = 2
};

AICORE inline uint16_t _getFFTSMsg(CVSyncMode mode, uint16_t flag_id, uint16_t base_const = 0x1) {
    return ((base_const & 0xf) + ((mode & 0x3) << 4) + ((flag_id & 0xf) << 8));
}

// Cross-core CV slot synchronization. Returns the CV comm slot for the current core/block.
// - block_idx: logical block index.
// - cv_comm_buf: global buffer sized by CV_COMM_SLOT_BYTES * block_rows.
// Template knobs allow overriding slot size and MAX_CORES if needed.
template <int CV_COMM_SLOT_BYTES = kCvCommSlotBytes, int CV_MAX_CORES = kCvMaxCores>
AICORE inline int TSYNC_CVID(int block_idx, __gm__ uint8_t *cv_comm_buf) {
    int comm_slot = block_idx;
#ifdef __DAV_CUBE__
    PTO_ASSERT(cv_comm_buf != nullptr, "cv_comm_buf must be non-null when CV comm is enabled on cube cores");
    comm_slot = static_cast<int>(get_coreid() & 0x7f);
    comm_slot %= CV_MAX_CORES;
    __gm__ volatile uint32_t *comm_slot_ptr = reinterpret_cast<__gm__ volatile uint32_t *>(
        cv_comm_buf + static_cast<std::size_t>(block_idx) * CV_COMM_SLOT_BYTES);
    comm_slot_ptr[0] = static_cast<uint32_t>(comm_slot);
    dcci(comm_slot_ptr, SINGLE_CACHE_LINE);
    dsb(DSB_DDR);
    ffts_cross_core_sync(PIPE_MTE2, _getFFTSMsg(CV_CORE_SYNC, CV_COMM_CTRL));
#elif defined(__DAV_VEC__)
    static_assert(CV_MAX_CORES > 0, "MAX_CORES must be positive");
    PTO_ASSERT(cv_comm_buf != nullptr, "cv_comm_buf must be non-null when CV comm is enabled on vector cores");
    __gm__ volatile uint32_t *comm_slot_ptr = reinterpret_cast<__gm__ volatile uint32_t *>(
        cv_comm_buf + static_cast<std::size_t>(block_idx) * CV_COMM_SLOT_BYTES);
    dcci(comm_slot_ptr, SINGLE_CACHE_LINE);
    wait_flag_dev(CV_COMM_CTRL);
    comm_slot = static_cast<int>(comm_slot_ptr[0]);
#endif
#ifdef _DEBUG
#ifdef __DAV_CUBE__
    cce::printf("Core %d Cube Block %d, comm_slot %d\n", get_coreid(), block_idx, comm_slot);
#elif defined(__DAV_VEC__)
    cce::printf("Core %d Vec Block %d, SubBlock %d, comm_slot %d\n", get_coreid(), block_idx,
        int(get_subblockid()), comm_slot);
#endif
#endif
    return comm_slot;
}
} // namespace pto

#endif
