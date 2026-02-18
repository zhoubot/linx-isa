/**
Copyright (c) 2025 Huawei Technologies Co., Ltd.
This program is free software, you can redistribute it and/or modify it under the terms and conditions of
CANN Open Software License Agreement Version 2.0 (the "License").
Please refer to the License for details. You may not use this file except in compliance with the License.
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
See LICENSE in the root of the software repository for the full text of the License.
*/

/*!
 * \file debug.h
 * \brief
 */

#ifndef PTO_DEBUG_H
#define PTO_DEBUG_H

#define DEBUG_CHECK(condition, message)                                                              \
    do {                                                                                             \
        if (!(condition)) {                                                                          \
            cce::printf(                                                                             \
                "[PTO][ASSERT] %s\n"                                                                 \
                "  Condition: %s\n"                                                                  \
                "  Location: %s:%d\n"                                                                \
                "  Hint: see docs/coding/debug.md (Fix recipes + assertion index)\n",                \
                (message), #condition, __FILE__, __LINE__);                                          \
            trap();                                                                                  \
        }                                                                                            \
    } while (0)

#ifdef _DEBUG
#define PTO_ASSERT(condition, message) DEBUG_CHECK(condition, message)
#else
#define PTO_ASSERT(condition, message) ((void)0)
#endif

#ifdef __CPU_SIM
  #include <algorithm>
  #include <climits>
  #include <iomanip>
  #include <iostream>
  #include <string>
  #include <type_traits>
  #include "pto/cpu/tile_offsets.hpp"
  #include "pto_tile.hpp"

  template<typename GT>
  void printRawGT(GT& tensor, const std::string name = "", int elementWidth=5, int maxR=INT32_MAX, int maxC=INT32_MAX) {
      constexpr int PRECISION = 2;
      auto rows = tensor.GetShape(pto::GlobalTensorDim::DIM_3);
      auto cols = tensor.GetShape(pto::GlobalTensorDim::DIM_4);
      auto stride3 = std::max(tensor.GetStride(pto::GlobalTensorDim::DIM_3), tensor.GetStride(pto::GlobalTensorDim::DIM_4));
      std::cout << name << ": " << tensor.GetShape(pto::GlobalTensorDim::DIM_0) << " x "
                << tensor.GetShape(pto::GlobalTensorDim::DIM_1) << " x "
                << tensor.GetShape(pto::GlobalTensorDim::DIM_2) << " x "
                << tensor.GetShape(pto::GlobalTensorDim::DIM_3) << " x "
                << tensor.GetShape(pto::GlobalTensorDim::DIM_4) << std::endl;
      for(int i=0; i<tensor.GetShape(pto::GlobalTensorDim::DIM_0); i++) {
          for(int j=0; j<tensor.GetShape(pto::GlobalTensorDim::DIM_1); j++) {
              for(int k=0; k<tensor.GetShape(pto::GlobalTensorDim::DIM_2); k++) {
                  std::cout << "    " << i << ", " << j << ", " << k << ", r, c:\n";

                  for(int y=0; y<rows && y<maxR; y++) {
                      for(int x=0; x<cols && x<maxC; x++) {
                          auto val = tensor.data()[i*tensor.GetStride(pto::GlobalTensorDim::DIM_0) + j*tensor.GetStride(pto::GlobalTensorDim::DIM_1)+k*tensor.GetStride(pto::GlobalTensorDim::DIM_2)+y*stride3+x];
                          if constexpr(std::is_integral_v<typename GT::DType>) {
                              std::cout << std::setw(elementWidth) << val << " ";
                          } else {
                              const auto v = (val < 1e-20 ? 0 : val);
                              std::cout << std::setw(elementWidth) << std::fixed << std::setprecision(PRECISION) << v << " ";
                          }
                      }
                      if(maxC < cols) {
                        std::cout << " ...";
                      }
                      std::cout << std::endl;
                  }
                  if(maxR < rows) {
                    std::cout << "..." << std::endl;
                  }
              }
          }
      }
  }

    template<typename TL>
    void printRawTile(TL& tile, const std::string name = "", int elementWidth=5, int maxR=INT32_MAX, int maxC=INT32_MAX) {
        constexpr int PRECISION = 2;
        std::cout << name << ": " << tile.GetValidRow() << " x " << tile.GetValidCol()
                  << " (Full: " << tile.Rows << " x " << tile.Cols << ") (RxC)" << std::endl;
        for(int y=0; y<tile.GetValidRow() && y<maxR; y++){
            for(int x=0; x<tile.GetValidCol() && x<maxC; x++) {
                if constexpr(std::is_integral_v<typename TL::DType>) {
                    std::cout << std::setw(elementWidth) << tile.data()[y*tile.Cols+x] << " ";
                } else {
                    const auto v = (tile.data()[y*tile.Cols+x] < 1e-20 ? 0 : tile.data()[y*tile.Cols+x]);
                    std::cout << std::setw(elementWidth) << std::fixed << std::setprecision(PRECISION) << v << " ";
                }
            }
            if(maxC < tile.GetValidCol()) {
              std::cout << " ...";
            }
            std::cout << std::endl;
        }
        if(maxR < tile.GetValidRow()) {
          std::cout << "..." << std::endl;
        }
    }
    
    template<typename TL>
    void printTile(TL& tile, const std::string name = "", int elementWidth=5, int maxR=INT32_MAX, int maxC=INT32_MAX) {
        constexpr int PRECISION = 2;
        std::cout << name << ": " << tile.GetValidRow() << " x " << tile.GetValidCol()
                  << " (Full: " << tile.Rows << " x " << tile.Cols << ") (RxC)" << std::endl;
        for(int y=0; y<tile.GetValidRow() && y<maxR; y++){
            for(int x=0; x<tile.GetValidCol() && x<maxC; x++) {
                auto offset = pto::GetTileElementOffset<TL>(y,x);
                if constexpr(std::is_integral_v<typename TL::DType>) {
                    std::cout << std::setw(elementWidth) << tile.data()[offset] << " ";
                } else {
                    const auto v = (tile.data()[offset] < 1e-20 ? 0 : tile.data()[offset]);
                    std::cout << std::setw(elementWidth) << std::fixed << std::setprecision(PRECISION) << v << " ";
                }
            }
            if(maxC < tile.GetValidCol()) {
              std::cout << " ...";
            }
            std::cout << std::endl;
        }
        if(maxR < tile.GetValidRow()) {
          std::cout << "..." << std::endl;
        }
    }    

    template<typename T>
    void printRawMemory(T * buf, size_t sz, const std::string name = "", int elementWidth=10, int elementsPerRow=8) {
        constexpr int PRECISION = 2;
        constexpr int DEFAULT_WIDTH = 6;
        std::cout << name << ": " << static_cast<void*>(buf) << std::endl;
        for(int i=0; i<sz; i++){
            if(i % elementsPerRow == 0) {
              std::cout << std::endl << std::setw(DEFAULT_WIDTH) << std::hex << i << ": " << std::dec;
            }
            if constexpr(std::is_integral_v<T>) {
                std::cout << std::setw(elementWidth) << buf[i] << " ";
            } else {
                const auto v = (buf[i] < 1e-20 ? 0 : buf[i]);
                std::cout << std::setw(elementWidth) << std::fixed << std::setprecision(PRECISION) << v << " ";
            }
        }
        std::cout << std::endl;
    }

#endif

#endif
