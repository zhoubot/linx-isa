/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

#ifndef TSYNC_CUSTOM_HPP
#define TSYNC_CUSTOM_HPP

#include <pto/common/type.hpp>
#include <pto/common/utils.hpp>

namespace pto {

// Operation types for TSync - identifies the producer/consumer operation
enum class SyncOpType : uint8_t {
    TSTORE_C2GM,  // Store (Cube core operation)
    TSTORE_V2GM,  // Store (Vector core operation)
    TLOAD         // Load operation (consumer operation)
};

// Compile-time direction inference based on producer/consumer ops
// TSTORE_C2GM (producer) + TLOAD (consumer) = Cube to Vector
// TSTORE_V2GM (producer) + TLOAD (consumer) = Vector to Cube
template <SyncOpType ProducerOp, SyncOpType ConsumerOp>
struct SyncTraits {
    // Direction is inferred from producer operation:
    // TSTORE_C2GM -> Cube produces (C2V)
    // TSTORE_V2GM -> Vector produces (V2C)
    static constexpr bool is_cube_to_vec = (ProducerOp == SyncOpType::TSTORE_C2GM);
    static constexpr bool is_vec_to_cube = (ProducerOp == SyncOpType::TSTORE_V2GM);
    
    static_assert(ConsumerOp == SyncOpType::TLOAD, "Consumer operation must be TLOAD");
    static_assert(is_cube_to_vec || is_vec_to_cube, 
                  "Producer must be either TSTORE_C2GM (Cube) or TSTORE_V2GM (Vector)");
};

namespace detail {
    template <int N>
    struct FlagIDTag {
        static constexpr int value = N;
    };
    
    // Base counter starts at 0 (user IDs start from 0 to 12)
    constexpr int kUserFlagIDStart = 0;
    constexpr int kMaxFlagID = 12;
    constexpr int kNumUserFlags = kMaxFlagID - kUserFlagIDStart + 1;  // 12 flags
}

/**
 * TSync - Lightweight synchronization primitive for intra-core dependencies
 * 
 * 
 * Usage with manual flag ID:
 *   constexpr TSync<TSTORE_C2GM, TLOAD> sync = {BUF0_QK_READY};
 * 
 * Forward dependency (producer -> consumer):
 *   Producer: sync.record()  // Signal data ready
 *   Consumer: sync.wait()    // Wait for data
 * 
 * Backward dependency (consumer -> producer):
 *   Producer: sync.allocate() // Wait for buffer space
 *   Consumer: sync.free()     // Signal buffer available
 * 
 * Template Parameters:
 *   ProducerOp: Producer operation (TSTORE_C2GM or TSTORE_V2GM)
 *   ConsumerOp: Consumer operation (TLOAD)
 */
template <SyncOpType ProducerOp, SyncOpType ConsumerOp>
struct TSync_Custom {
    using Traits = SyncTraits<ProducerOp, ConsumerOp>;
    static constexpr bool is_c2v = Traits::is_cube_to_vec;
    static constexpr bool is_v2c = Traits::is_vec_to_cube;
    
    uint16_t flag_id;  // FFTS flag ID for cross-core synchronization
    
    // Forward dependency: record (producer) and wait (consumer)

    /**
     * record - Producer signals that data is ready
     * Called by the producer after completing the operation (TSTORE_C2GM or TSTORE_V2GM)
     */
    AICORE inline void record() const {
        if constexpr (is_c2v) {
            // Cube produces, Vector consumes
            ffts_cross_core_sync(PIPE_FIX, _getFFTSMsg(CV_CORE_SYNC, flag_id));
        } else { // is_v2c
            // Vector produces, Cube consumes
            ffts_cross_core_sync(PIPE_MTE3, _getFFTSMsg(CV_CORE_SYNC, flag_id));
        }
    }
    
    /**
     * wait - Consumer waits for data to be ready
     * Called by the consumer before accessing the data (TLOAD)
     */
    AICORE inline void wait() const {
        if constexpr (is_c2v) {
            // Vector waits for Cube
            wait_flag_dev(flag_id);
        } else { // is_v2c
            // Cube waits for Vector
            wait_flag_dev(flag_id);
        }
    }
    
    // Backward dependency: allocate (producer) and free (consumer)
    
    /**
     * allocate - Producer waits for buffer space to be available
     * Called by the producer before writing new data
     */
    AICORE inline void allocate() const {
        if constexpr (is_c2v) {
            // Cube waits for Vector to free buffer
            wait_flag_dev(flag_id + 1);
        } else { // is_v2c
            // Vector waits for Cube to free buffer
            wait_flag_dev(flag_id + 1);
        }
    }
    
    /**
     * free - Consumer signals that buffer space is available
     * Called by the consumer after consuming data
     */
    AICORE inline void free() const {
        if constexpr (is_c2v) {
            // Vector frees buffer for Cube
            ffts_cross_core_sync(PIPE_MTE2, _getFFTSMsg(CV_CORE_SYNC, flag_id + 1));
        } else { // is_v2c
            // Cube frees buffer for Vector
            ffts_cross_core_sync(PIPE_MTE2, _getFFTSMsg(CV_CORE_SYNC, flag_id + 1));
        }
    }
};


} // namespace pto

#endif // TSYNC_HPP
