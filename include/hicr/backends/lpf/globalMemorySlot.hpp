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
 * @file globalMemorySlot.hpp
 * @brief Provides a definition for the global memory slot class for the LPF backend
 * @author K. Dichev
 * @date 25/10/2023
 */
#pragma once

#include <lpf/core.h>
#include <hicr/core/globalMemorySlot.hpp>
#include <hicr/core/localMemorySlot.hpp>
#include <utility>

namespace HiCR::backend::lpf
{

/**
 * This class is the definition for a Memory Slot resource for the LPF backend
 */
class GlobalMemorySlot final : public HiCR::GlobalMemorySlot
{
  public:

  /**
   * Constructor for a MemorySlot class for the LPF backend
   * @param[in] rank  Rank
   * @param[in] lpfMemSlot LPF slot this HiCR slot is associated with
   * @param[in] lpfSwapSlot LPF swap slot for global acquire/release calls this HiCR slot is associated with
   * @param[in] globalTag The global tag associated to this global memory slot (for exchange purposes)
   * @param[in] globalKey The global key associated to this global memory slot (for exchange purposes
   * @param[in] sourceLocalMemorySlot The local memory slot (if applicable) from whence this global memory slot is created
   */
  GlobalMemorySlot(lpf_pid_t                                 rank,
                   lpf_memslot_t                             lpfMemSlot,
                   lpf_memslot_t                             lpfSwapSlot,
                   const HiCR::GlobalMemorySlot::tag_t       globalTag             = 0,
                   const HiCR::GlobalMemorySlot::globalKey_t globalKey             = 0,
                   std::shared_ptr<HiCR::LocalMemorySlot>    sourceLocalMemorySlot = nullptr)
    : HiCR::GlobalMemorySlot(globalTag, globalKey, std::move(sourceLocalMemorySlot)),
      _rank(rank),
      _lpfMemSlot(lpfMemSlot),
      _lpfSwapSlot(lpfSwapSlot)
  {}

  /**
   * Default destructor
   */
  ~GlobalMemorySlot() override = default;

  /**
   * Returns the rank to which this memory slot belongs
   *
   * \return The rank to which this memory slot belongs
   */
  const lpf_pid_t getRank() { return _rank; }

  /**
   * Get LPF slot associated with this HiCR slot
   * @return LPF slot
   */
  [[nodiscard]] lpf_memslot_t getLPFSlot() const { return _lpfMemSlot; }

  /**
   * Get LPF swap slot associated with this HiCR slot. This slot is only used for
   * acquire/release operations on the HiCR slot.
   * @return LPF slot
   */
  [[nodiscard]] lpf_memslot_t getLPFSwapSlot() const { return _lpfSwapSlot; }

  private:

  /**
   * Remembers the MPI rank this memory slot belongs to
   */
  lpf_pid_t _rank;

  /**
   * Internal LPF slot represented by this HiCR memory slot
   */
  const lpf_memslot_t _lpfMemSlot;

  /**
   * Internal LPF slot only used for global acquire/release
   * operations. It relies on IB Verbs atomic compare-and-swap
   */
  const lpf_memslot_t _lpfSwapSlot;
};

} // namespace HiCR::backend::lpf
