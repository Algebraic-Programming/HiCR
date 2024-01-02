/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file memoryManager.hpp
 * @brief This file implements the memory manager support for the sequential backend
 * @author S. M. Martin
 * @date 14/8/2023
 */

#pragma once

#include <atomic>
#include <backends/sequential/L0/memorySpace.hpp>
#include <hicr/L1/memoryManager.hpp>
#include <hicr/common/definitions.hpp>
#include <hicr/common/exceptions.hpp>

namespace HiCR 
{

namespace backend
{

namespace sequential
{

namespace L1
{

/**
 * Implementation of the memory manager for the sequential backend
 */
class MemoryManager final : public HiCR::L1::MemoryManager
{
  public:

  /**
   * The constructor is employed to create the barriers required to coordinate threads
   */
  MemoryManager() : HiCR::L1::MemoryManager() {}
  ~MemoryManager() = default;

  private:

  /**
   * Allocates memory in the current memory space (whole system)
   *
   * \param[in] memorySpace Memory space in which to perform the allocation.
   * \param[in] size Size of the memory slot to create
   * \returns The pointer of the newly allocated memory slot
   */
  __USED__ inline HiCR::L0::LocalMemorySlot *allocateLocalMemorySlotImpl(HiCR::L0::MemorySpace* memorySpace, const size_t size) override
  {
    // Getting up-casted pointer for the MPI instance
    auto m = dynamic_cast<const L0::MemorySpace *>(memorySpace);

    // Checking whether the execution unit passed is compatible with this backend
    if (m == NULL) HICR_THROW_LOGIC("The passed memory space is not supported by this memory manager\n");

    // Atempting to allocate the new memory slot
    auto ptr = malloc(size);

    // Check whether it was successful
    if (ptr == NULL) HICR_THROW_RUNTIME("Could not allocate memory of size %lu", size);

    // Creating and returning new memory slot
    return  new HiCR::L0::LocalMemorySlot(ptr, size, memorySpace);
  }

  /**
   * Associates a pointer locally-allocated manually and creates a local memory slot with it
   * \param[in] memorySpace The memory space to register the new memory slot in
   * \param[in] ptr Pointer to the local memory space
   * \param[in] size Size of the memory slot to register
   * \return A newly created memory slot
   */
  __USED__ inline HiCR::L0::LocalMemorySlot *registerLocalMemorySlotImpl(HiCR::L0::MemorySpace* memorySpace, void *const ptr, const size_t size) override
  {
    // Returning new memory slot pointer
    return new HiCR::L0::LocalMemorySlot(ptr, size, memorySpace);
  }

  /**
   * De-registers a memory slot previously registered
   *
   * \param[in] memorySlot Memory slot to deregister.
   */
  __USED__ inline void deregisterLocalMemorySlotImpl(HiCR::L0::LocalMemorySlot *memorySlot) override
  {
    // Nothing to do here for this backend
  }

  /**
   * Backend-internal implementation of the freeLocalMemorySlot function
   *
   * \param[in] memorySlot Local memory slot to free up. It becomes unusable after freeing.
   */
  __USED__ inline void freeLocalMemorySlotImpl(HiCR::L0::LocalMemorySlot *memorySlot) override
  {
    if (memorySlot->getPointer() == NULL) HICR_THROW_RUNTIME("Invalid memory slot(s) provided. It either does not exit or represents a NULL pointer.");

    free(memorySlot->getPointer());
  }
};

} // namespace L1

} // namespace sequential

} // namespace backend

} // namespace HiCR
