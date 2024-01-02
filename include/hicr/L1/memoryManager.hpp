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

#include <hicr/L0/localMemorySlot.hpp>  
#include <hicr/L0/globalMemorySlot.hpp> 
#include <hicr/L0/memorySpace.hpp>
#include <hicr/common/definitions.hpp>
#include <hicr/common/exceptions.hpp>
#include <map>

namespace HiCR
{

namespace L1
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
  __USED__ inline L0::LocalMemorySlot *allocateLocalMemorySlot(L0::MemorySpace* memorySpace, const size_t size)
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
  virtual L0::LocalMemorySlot *registerLocalMemorySlot(L0::MemorySpace* memorySpace, void *const ptr, const size_t size)
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
  __USED__ inline void deregisterLocalMemorySlot(L0::LocalMemorySlot *const memorySlot)
  {
    // Decreasing memory space usage
    memorySlot->getMemorySpace()->decreaseUsage(memorySlot->getSize());

    // Calling internal implementation
    deregisterLocalMemorySlotImpl(memorySlot);
  }

  /**
   * Frees up a memory slot reserved from this memory space
   *
   * \param[in] memorySlot Memory slot to free up. It becomes unusable after freeing.
   */
  __USED__ inline void freeLocalMemorySlot(L0::LocalMemorySlot *memorySlot)
  {
    // Decreasing memory space usage
    memorySlot->getMemorySpace()->decreaseUsage(memorySlot->getSize());

    // Actually freeing up slot
    freeLocalMemorySlotImpl(memorySlot);
  }

  protected:

  /**
   * Backend-internal implementation of the queryLocalMemorySlot function
   *
   * \param[in] memorySpace Memory space to allocate memory from
   * \param[in] size Size of the memory slot to create
   * \return The internal pointer associated to the local memory slot
   */
  virtual L0::LocalMemorySlot *allocateLocalMemorySlotImpl(L0::MemorySpace* memorySpace, const size_t size) = 0;

  /**
   * Backend-internal implementation of the registerLocalMemorySlot function
   *
   * \param[in] memorySpace Memory space to register memory in
   * \param[in] ptr Pointer to the start of the memory slot
   * \param[in] size Size of the memory slot to create
   * \return A newly created memory slot
   */
  virtual L0::LocalMemorySlot *registerLocalMemorySlotImpl(L0::MemorySpace* memorySpace, void *const ptr, const size_t size) = 0;

  /**
   * Backend-internal implementation of the freeLocalMemorySlot function
   *
   * \param[in] memorySlot Local memory slot to free up. It becomes unusable after freeing.
   */
  virtual void freeLocalMemorySlotImpl(L0::LocalMemorySlot *memorySlot) = 0;

  /**
   * Backend-internal implementation of the deregisterMemorySlot function
   *
   * \param[in] memorySlot Memory slot to deregister.
   */
  virtual void deregisterLocalMemorySlotImpl(L0::LocalMemorySlot *memorySlot) = 0;
};

} // namespace L1

} // namespace HiCR
