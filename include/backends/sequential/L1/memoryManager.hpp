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

#include <memory>
#include <hicr/definitions.hpp>
#include <hicr/exceptions.hpp>
#include <hicr/L1/memoryManager.hpp>
#include <backends/sequential/L0/memorySpace.hpp>

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

  __USED__ inline std::shared_ptr<HiCR::L0::LocalMemorySlot> allocateLocalMemorySlotImpl(std::shared_ptr<HiCR::L0::MemorySpace> memorySpace, const size_t size) override
  {
    // Getting up-casted pointer for the MPI instance
    auto m = dynamic_pointer_cast<sequential::L0::MemorySpace>(memorySpace);

    // Checking whether the execution unit passed is compatible with this backend
    if (m == NULL) HICR_THROW_LOGIC("The passed memory space is not supported by this memory manager\n");

    // Atempting to allocate the new memory slot
    auto ptr = malloc(size);

    // Check whether it was successful
    if (ptr == NULL) HICR_THROW_RUNTIME("Could not allocate memory of size %lu", size);

    // Creating and returning new memory slot
    return std::make_shared<HiCR::L0::LocalMemorySlot>(ptr, size, memorySpace);
  }

  __USED__ inline std::shared_ptr<HiCR::L0::LocalMemorySlot> registerLocalMemorySlotImpl(std::shared_ptr<HiCR::L0::MemorySpace> memorySpace, void *const ptr, const size_t size) override
  {
    // Returning new memory slot pointer
    return std::make_shared<HiCR::L0::LocalMemorySlot>(ptr, size, memorySpace);
  }

  __USED__ inline void deregisterLocalMemorySlotImpl(std::shared_ptr<HiCR::L0::LocalMemorySlot> memorySlot) override
  {
    // Nothing to do here for this backend
  }

  __USED__ inline void freeLocalMemorySlotImpl(std::shared_ptr<HiCR::L0::LocalMemorySlot> memorySlot) override
  {
    if (memorySlot->getPointer() == NULL) HICR_THROW_RUNTIME("Invalid memory slot(s) provided. It either does not exit or represents a NULL pointer.");

    free(memorySlot->getPointer());
  }
};

} // namespace L1

} // namespace sequential

} // namespace backend

} // namespace HiCR
