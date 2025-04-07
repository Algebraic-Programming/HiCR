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
 * @file hwloc/globalMemorySlot.hpp
 * @brief Provides a definition for the global memory slot class for the  HWLoc-based backend
 * @author S. M. Martin
 * @date 19/10/2023
 */
#pragma once

#include <mutex>
#include <hicr/core/localMemorySlot.hpp>
#include <hicr/core/globalMemorySlot.hpp>
#include <utility>

namespace HiCR::backend::hwloc
{

/**
 * This class represents an abstract definition for a global Memory Slot resource for the host (CPU) backends
 *
 * It uses mutexes to enforce the mutual exclusion logic
 */
class GlobalMemorySlot final : public HiCR::GlobalMemorySlot
{
  public:

  /**
   * Constructor for a MemorySlot class for the MPI backend
   *
   * \param[in] globalTag For global memory slots, indicates the subset of global memory slots this belongs to
   * \param[in] globalKey Unique identifier for that memory slot that this slot occupies.
   * \param[in] sourceLocalMemorySlot The local memory slot (if applicable) from whence this global memory slot is created
   */
  GlobalMemorySlot(const HiCR::GlobalMemorySlot::tag_t       globalTag             = 0,
                   const HiCR::GlobalMemorySlot::globalKey_t globalKey             = 0,
                   std::shared_ptr<HiCR::LocalMemorySlot>    sourceLocalMemorySlot = nullptr)
    : HiCR::GlobalMemorySlot(globalTag, globalKey, std::move(sourceLocalMemorySlot))
  {}

  ~GlobalMemorySlot() override = default;

  /**
   * Attempts to lock memory lock using its mutex object
   *
   * This function never blocks the caller
   *
   * @return True, if successful; false, otherwise.
   */
  __INLINE__ bool trylock() { return _mutex.try_lock(); }

  /**
   * Attempts to lock memory lock using its mutex object
   *
   * This function might block the caller if the memory slot is already locked
   */
  __INLINE__ void lock() { _mutex.lock(); }

  /**
   * Unlocks the memory slot, if previously locked by the caller
   */
  __INLINE__ void unlock() { _mutex.unlock(); }

  private:

  /**
   * Internal memory slot mutex to enforce lock acquisition
   */
  std::mutex _mutex;
};

} // namespace HiCR::backend::hwloc
