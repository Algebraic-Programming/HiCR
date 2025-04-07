/*
 *   Copyright 2025 Huawei Technologies Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file localMemorySlot.hpp
 * @brief Provides a definition for the local memory slot class for the LPF backend
 * @author K. Dichev
 * @date 25/10/2023
 */
#pragma once

#include <memory>
#include <lpf/core.h>
#include <hicr/core/localMemorySlot.hpp>
#include <hicr/core/memorySpace.hpp>
#include <utility>

#include <hicr/backends/lpf/common.hpp>

namespace HiCR::backend::lpf
{

/**
 * This class is the definition for a Memory Slot resource for the LPF backend
 */
class LocalMemorySlot final : public HiCR::LocalMemorySlot
{
  public:

  /**
   * Constructor for a MemorySlot class for the LPF backend
   * @param[in] lpfMemSlot LPF slot this HiCR slot is associated with
   * @param[in] pointer Pointer to the memory address associated with this HiCR slot
   * @param[in] size Comm size
   * @param[in] memorySpace The memory space from whence this memory slot was created
   */
  LocalMemorySlot(lpf_memslot_t lpfMemSlot, void *const pointer, const size_t size, std::shared_ptr<HiCR::MemorySpace> memorySpace)
    : HiCR::LocalMemorySlot(pointer, size, std::move(memorySpace)),
      _lpfMemSlot(lpfMemSlot)

  {}

  /**
   * Default destructor
   */
  ~LocalMemorySlot() override = default;

  /**
   * Get internal LPF slot (lpf_memslot_t) associated with this HiCR slot
   * @return LPF slot
   */
  [[nodiscard]] lpf_memslot_t getLPFSlot() const { return _lpfMemSlot; }

  /**
   * @param[in] lpfMemSlot The LPF memory slot
   * Set internal LPF slot (lpf_memslot_t) associated with this HiCR slot
   */
  void setLPFSlot(lpf_memslot_t lpfMemSlot) { _lpfMemSlot = lpfMemSlot; }

  /**
   * Getter function for the memory slot's swap value pointer
   * \returns The memory slot's swap value internal pointer
   */
  __INLINE__ void *getLPFSwapPointer() { return &_swapValue; }

  private:

  /**
   * Internal LPF slot represented by this HiCR memory slot. It may be
   * modified during its lifecycle since a promoted slot needs to update
   * its _lpfMemSlot
   */
  lpf_memslot_t _lpfMemSlot;

  /**
   * Internal LPF swap value for acquire/release of global slots.
   * Currently 0ULL = released/available; 1Ull = acquired
   */
  uint64_t _swapValue{0ULL};
};

} // namespace HiCR::backend::lpf
