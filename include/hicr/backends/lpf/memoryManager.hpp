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
 * @file memoryManager.hpp
 * @brief This is the LPF backend implementation, which is currently tested with
 * the LPF implementation under https://github.com/Algebraic-Programming/LPF/tree/noc_extension
 * @author K. Dichev
 * @date 24/10/2023
 */

#pragma once

#include <cstring>
#include <lpf/collectives.h>
#include <lpf/core.h>
#include <hicr/core/memoryManager.hpp>
#include <hicr/backends/hwloc/memorySpace.hpp>
#include "localMemorySlot.hpp"

namespace HiCR::backend::lpf
{

/**
 * Implementation of the LPF backend
 *
 * The only LPF engine currently of interest is the IB Verbs engine (see above for branch and hash)
 */
class MemoryManager final : public HiCR::MemoryManager
{
  private:

  const lpf_t _lpf;

  /**
   * Map of global slot id and MPI windows
   */

  public:

  /**
   * Constructor of the LPF memory manager
   * @param[in] lpf LPF context
   * \note The decision to resize memory register in the constructor
   * is because this call requires lpf_sync to become effective.
   * This makes it almost impossible to do local memory registrations with LPF.
   * On the other hand, the resize message queue could also be locally
   * made, and placed elsewhere.
   */
  MemoryManager(lpf_t lpf)
    : HiCR::MemoryManager(),
      _lpf(lpf)
  {}

  /**
   * Associates a pointer locally-allocated manually and creates a local memory slot with it
   *
   * \param[in] ptr Pointer to the local memory space
   * \param[in] size Size of the memory slot to register
   * \param[in] memorySpace The memory space onto which to register the new memory slot
   * \return A newly created memory slot
   */
  __INLINE__ std::shared_ptr<HiCR::LocalMemorySlot> registerLocalMemorySlotImpl(std::shared_ptr<HiCR::MemorySpace> memorySpace, void *const ptr, const size_t size) override
  {
    // Getting up-casted pointer for the MPI instance
    auto m = dynamic_pointer_cast<hwloc::MemorySpace>(memorySpace);

    // Checking whether the execution unit passed is compatible with this backend
    if (m == nullptr) HICR_THROW_LOGIC("The passed memory space is not supported by this memory manager\n");

    lpf_memslot_t lpfSlot = LPF_INVALID_MEMSLOT;
    auto          rc      = lpf_register_local(_lpf, ptr, size, &lpfSlot);
    if (rc != LPF_SUCCESS) HICR_THROW_RUNTIME("LPF Memory Manager: lpf_register_local failed");

    // Creating new memory slot object
    auto memorySlot = std::make_shared<lpf::LocalMemorySlot>(lpfSlot, ptr, size, memorySpace);

    return memorySlot;
  }

  /**
   * De-registers a memory slot previously registered
   *
   * \param[in] memorySlot Pointer to the memory slot to deregister.
   */
  __INLINE__ void deregisterLocalMemorySlotImpl(std::shared_ptr<HiCR::LocalMemorySlot> memorySlot) override
  {
    // Getting up-casted pointer for the memory slot
    auto slot = dynamic_pointer_cast<lpf::LocalMemorySlot>(memorySlot);

    // Checking whether the memory slot is compatible with this backend
    if (slot == nullptr) HICR_THROW_LOGIC("The passed memory slot is not supported by this backend\n");

    // Deregistering memory slot from the LPF backend
    lpf_deregister(_lpf, slot->getLPFSlot());
  }

  /**
   * Allocates memory in the current memory space (whole system)
   *
   * \param[in] memorySpace Memory space in which to perform the allocation.
   * \param[in] size Size of the memory slot to create
   * \returns The address of the newly allocated memory slot
   */
  __INLINE__ std::shared_ptr<HiCR::LocalMemorySlot> allocateLocalMemorySlotImpl(std::shared_ptr<HiCR::MemorySpace> memorySpace, const size_t size) override
  {
    // Getting up-casted pointer for the LPF instance
    auto m = dynamic_pointer_cast<hwloc::MemorySpace>(memorySpace);

    // Checking whether the execution unit passed is compatible with this backend
    if (m == nullptr) HICR_THROW_LOGIC("The passed memory space is not supported by this memory manager\n");

    // Storage for the new pointer
    const size_t MIN_BYTES = 32;
    const auto   newSize   = std::max(size, MIN_BYTES);
    void        *ptr       = malloc(newSize);
    if (ptr == nullptr) HICR_THROW_RUNTIME("Could not allocate memory of size %lu", newSize);

    // Update the memory usage for the memory space
    memorySpace->increaseUsage(newSize - size);
    // Creating and returning new memory slot
    return registerLocalMemorySlotImpl(memorySpace, ptr, newSize);
  }

  /**
   * Frees up a local memory slot reserved from this memory space
   *
   * \param[in] memorySlot Local memory slot to free up. It becomes unusable after freeing.
   */
  __INLINE__ void freeLocalMemorySlotImpl(std::shared_ptr<HiCR::LocalMemorySlot> memorySlot) override
  {
    // Getting up-casted pointer for the memory slot
    auto slot = dynamic_pointer_cast<lpf::LocalMemorySlot>(memorySlot);

    // Checking whether the memory slot is compatible with this backend
    if (slot == nullptr) HICR_THROW_LOGIC("The passed memory slot is not supported by this backend\n");

    // Getting memory slot pointer
    const auto pointer = slot->getPointer();

    // Checking whether the pointer is valid
    if (slot == nullptr) HICR_THROW_RUNTIME("Invalid memory slot(s) provided. It either does not exist or represents a NULL pointer.");

    // First, deregistering LPF memory slot
    deregisterLocalMemorySlotImpl(memorySlot);

    // Deallocating memory
    free(pointer);
  }
};

} // namespace HiCR::backend::lpf
