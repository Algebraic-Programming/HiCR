/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file memoryManager.hpp
 * @brief This file implements the memory manager class for the MPI backend
 * @author S. M. Martin
 * @date 11/9/2023
 */

#pragma once

#include <mpi.h>
#include <hicr/definitions.hpp>
#include <hicr/L0/localMemorySlot.hpp>
#include <hicr/L1/memoryManager.hpp>
#include <backends/host/L0/memorySpace.hpp>

namespace HiCR
{

namespace backend
{

namespace mpi
{

namespace L1
{

/**
 * Implementation of the HiCR MPI backend
 *
 * This backend is very useful for testing other HiCR modules in isolation (unit tests) without involving the use of threading, which might incur side-effects
 */
class MemoryManager final : public HiCR::L1::MemoryManager
{
  public:

  /**
   * Constructor for the mpi backend.
   */
  MemoryManager() : HiCR::L1::MemoryManager() {}
  ~MemoryManager() = default;

  /**
   * Allocates memory in the current memory space (whole system) using MPI_Alloc_mem
   * This method, as opposed to a normal malloc ensures portability for all MPI implementations
   *
   * \param[in] memorySpace Memory space in which to perform the allocation.
   * \param[in] size Size of the memory slot to create
   * \returns The address of the newly allocated memory slot
   */
  __USED__ inline std::shared_ptr<HiCR::L0::LocalMemorySlot> allocateLocalMemorySlotImpl(std::shared_ptr<HiCR::L0::MemorySpace> memorySpace, const size_t size) override
  {
    // Getting up-casted pointer for the MPI instance
    auto m = dynamic_pointer_cast<host::L0::MemorySpace>(memorySpace);

    // Checking whether the execution unit passed is compatible with this backend
    if (m == NULL) HICR_THROW_LOGIC("The passed memory space is not supported by this memory manager\n");

    // Storage for the new pointer
    void *ptr = NULL;

    // Attempting to allocate the new memory slot
    auto status = MPI_Alloc_mem(size, MPI_INFO_NULL, &ptr);

    // Check whether it was successful
    if (status != MPI_SUCCESS || ptr == NULL) HICR_THROW_RUNTIME("Could not allocate memory of size %lu", size);

    // Creating and returning new memory slot
    return registerLocalMemorySlotImpl(memorySpace, ptr, size);
  }

  __USED__ inline void freeLocalMemorySlotImpl(std::shared_ptr<HiCR::L0::LocalMemorySlot> memorySlot) override
  {
    // Getting memory slot pointer
    const auto pointer = memorySlot->getPointer();

    // Checking whether the pointer is valid
    if (pointer == NULL) HICR_THROW_RUNTIME("Invalid memory slot(s) provided. It either does not exist or represents a NULL pointer.");

    // Deallocating memory using MPI's free mechanism
    auto status = MPI_Free_mem(pointer);

    // Check whether it was successful
    if (status != MPI_SUCCESS) HICR_THROW_RUNTIME("Could not free memory slot (ptr: 0x%lX, size: %lu)", pointer, memorySlot->getSize());
  }

  __USED__ inline std::shared_ptr<HiCR::L0::LocalMemorySlot> registerLocalMemorySlotImpl(std::shared_ptr<HiCR::L0::MemorySpace> memorySpace, void *const ptr, const size_t size) override
  {
    // Creating new memory slot object
    auto memorySlot = std::make_shared<HiCR::L0::LocalMemorySlot>(ptr, size, memorySpace);

    // Returning new memory slot pointer
    return memorySlot;
  }

  __USED__ inline void deregisterLocalMemorySlotImpl(std::shared_ptr<HiCR::L0::LocalMemorySlot> memorySlot) override
  {
    // Nothing to do here for this backend
  }
};

} // namespace L1

} // namespace mpi

} // namespace backend

} // namespace HiCR
