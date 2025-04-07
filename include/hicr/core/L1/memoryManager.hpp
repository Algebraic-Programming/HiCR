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

/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file memoryManager.hpp
 * @brief Provides a definition for the base backend's memory manager class.
 * @author S. M. Martin
 * @date 11/10/2023
 */

#pragma once

#include <map>
#include <memory>
#include <hicr/core/L0/localMemorySlot.hpp>
#include <hicr/core/L0/globalMemorySlot.hpp>
#include <hicr/core/L0/memorySpace.hpp>
#include <hicr/core/definitions.hpp>
#include <hicr/core/exceptions.hpp>

namespace HiCR::L1
{

/**
 * Encapsulates a HiCR Backend Memory Manager.
 *
 * Backends represent plugins to HiCR that provide support for a communication or device library. By adding new plugins developers extend HiCR's support for new hardware and software technologies.
 *
 * Backends need to fulfill the abstract virtual functions described here, so that HiCR can perform memory allocation/free/register operations on the supported device/network library.
 *
 */
class MemoryManager
{
  public:

  /**
   * Default destructor
   */
  virtual ~MemoryManager() = default;

  /**
   * Allocates a local memory slot in the specified memory space
   *
   * \param[in] memorySpace Memory space in which to perform the allocation.
   * \param[in] size Size of the memory slot to create
   * \returns The pointer of the newly allocated memory slot
   */
  __INLINE__ std::shared_ptr<L0::LocalMemorySlot> allocateLocalMemorySlot(const std::shared_ptr<L0::MemorySpace> &memorySpace, const size_t size)
  {
    // Increasing memory space usage
    memorySpace->increaseUsage(size);

    // Creating new memory slot structure
    auto newMemSlot = allocateLocalMemorySlotImpl(memorySpace, size);

    // Returning the id of the new memory slot
    return newMemSlot;
  }

  /**
   * Registers a local memory slot from a given address
   *
   * \param[in] memorySpace Memory space in which to perform the allocation.
   * \param[in] ptr Pointer to the start of the memory slot
   * \param[in] size Size of the memory slot to create
   * \returns The pointer of the newly registered memory slot
   */
  virtual std::shared_ptr<L0::LocalMemorySlot> registerLocalMemorySlot(const std::shared_ptr<HiCR::L0::MemorySpace> &memorySpace, void *const ptr, const size_t size)
  {
    // Increasing memory space usage
    memorySpace->increaseUsage(size);

    // Creating new memory slot structure
    auto newMemSlot = registerLocalMemorySlotImpl(memorySpace, ptr, size);

    // Returning the id of the new memory slot
    return newMemSlot;
  }

  /**
   * De-registers a previously registered local memory slot
   *
   * \param[in] memorySlot Memory slot to deregister.
   */
  __INLINE__ void deregisterLocalMemorySlot(const std::shared_ptr<HiCR::L0::LocalMemorySlot> &memorySlot)
  {
    // Decreasing memory space usage
    memorySlot->getMemorySpace()->decreaseUsage(memorySlot->getSize());

    // Calling internal implementation
    deregisterLocalMemorySlotImpl(memorySlot);
  }

  /**
   * Fills a memory slot with a given value.
   * This is a blocking operation.
   *
   * Like standard memset, this function fills the memory slot with the given value, starting from the beginning of the memory slot.
   * No bounds checking is performed, so the caller must ensure that the memory slot is large enough to hold the requested number of bytes,
   * should the size parameter be provided.
   *
   * \param[in] memorySlot Memory slot to fill
   * \param[in] value Value to fill the memory slot with
   * \param[in] size Number of bytes to fill, from the start of the memory slot. If not provided, the whole memory slot is filled.
   *            \note: The size parameter is not checked against the memory slot's size, so the caller must ensure that the size is valid.
   */
  __INLINE__ void memset(const std::shared_ptr<HiCR::L0::LocalMemorySlot> &memorySlot, int value, size_t size)
  {
    // Checking whether the pointer is valid
    if (memorySlot->getPointer() == nullptr) HICR_THROW_RUNTIME("Invalid memory slot provided. It either does not exist or represents a NULL pointer.");

    memsetImpl(memorySlot, value, size);
  }

  /**
   * Frees up a memory slot reserved from this memory space
   *
   * \param[in] memorySlot Memory slot to free up. It becomes unusable after freeing.
   */
  __INLINE__ void freeLocalMemorySlot(const std::shared_ptr<HiCR::L0::LocalMemorySlot> &memorySlot)
  {
    // Decreasing memory space usage
    memorySlot->getMemorySpace()->decreaseUsage(memorySlot->getSize());

    // Actually freeing up slot
    freeLocalMemorySlotImpl(memorySlot);
  }

  protected:

  /**
   * Backend-internal implementation of the allocateLocalMemorySlot function
   *
   * \param[in] memorySpace Memory space to allocate memory from
   * \param[in] size Size of the memory slot to create
   * \return The internal pointer associated to the local memory slot
   */
  virtual std::shared_ptr<HiCR::L0::LocalMemorySlot> allocateLocalMemorySlotImpl(std::shared_ptr<HiCR::L0::MemorySpace> memorySpace, const size_t size) = 0;

  /**
   * Backend-internal implementation of the registerLocalMemorySlot function
   *
   * \param[in] memorySpace Memory space to register memory in
   * \param[in] ptr Pointer to the start of the memory slot
   * \param[in] size Size of the memory slot to create
   * \return A newly created memory slot
   */
  virtual std::shared_ptr<L0::LocalMemorySlot> registerLocalMemorySlotImpl(std::shared_ptr<HiCR::L0::MemorySpace> memorySpace, void *const ptr, const size_t size) = 0;

  /**
   * Backend-internal implementation of the memset function
   *
   * \param[in] memorySlot Memory slot to fill
   * \param[in] value Value to fill the memory slot with
   * \param[in] size Number of bytes to fill, from the start of the memory slot
   */
  virtual void memsetImpl(const std::shared_ptr<HiCR::L0::LocalMemorySlot> memorySlot, int value, size_t size)
  {
    // Default implementation: using standard memset. Almost all backends can use this so no reason to reimplement it everywhere.
    std::memset(memorySlot->getPointer(), value, size);
  }

  /**
   * Backend-internal implementation of the freeLocalMemorySlot function
   *
   * \param[in] memorySlot Local memory slot to free up. It becomes unusable after freeing.
   */
  virtual void freeLocalMemorySlotImpl(std::shared_ptr<HiCR::L0::LocalMemorySlot> memorySlot) = 0;

  /**
   * Backend-internal implementation of the deregisterMemorySlot function
   *
   * \param[in] memorySlot Memory slot to deregister.
   */
  virtual void deregisterLocalMemorySlotImpl(std::shared_ptr<HiCR::L0::LocalMemorySlot> memorySlot) = 0;
};

} // namespace HiCR::L1
