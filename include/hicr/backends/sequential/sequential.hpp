/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file sequential.hpp
 * @brief This is a minimal backend for sequential execution support
 * @author S. M. Martin
 * @date 11/9/2023
 */

#pragma once

#include <cstring>
#include <future>
#include <stdio.h>

#include <hicr/backend.hpp>
#include <hicr/backends/sequential/process.hpp>

namespace HiCR
{

namespace backend
{

namespace sequential
{

/**
 * Implementation of the HiCR Sequential backend
 *
 * This backend is very useful for testing other HiCR modules in isolation (unit tests) without involving the use of threading, which might incur side-effects
 */
class Sequential final : public Backend
{

  private:

  /**
   * This stores the total system memory to check that allocations do not exceed it
   */
  size_t _totalSystemMem = 0;


  /**
   * This function returns the available allocatable size in the current system RAM
   *
   * @param[in] memorySpace Always zero, represents the system's RAM
   * @return The allocatable size within the system
   */
  __USED__ inline size_t getMemorySpaceSizeImpl(const memorySpaceId_t memorySpace) const override
  {
    return _totalSystemMem;
  }

  /**
   * This function returns the system physical memory size, which is what matters for a sequential program
   *
   * This is adapted from https://stackoverflow.com/a/2513561
   */
  __USED__ inline static size_t getTotalSystemMemory()
  {
    size_t pages = sysconf(_SC_PHYS_PAGES);
    size_t page_size = sysconf(_SC_PAGE_SIZE);
    return pages * page_size;
  }

  /**
   * Sequential backend implementation that returns a single compute element.
   */
  __USED__ inline computeResourceList_t queryComputeResourcesImpl() override
  {
    // Only a single processing unit is created
    return computeResourceList_t({0});
  }

  /**
   * Sequential backend implementation that returns a single memory space representing the entire RAM host memory.
   */
  __USED__ inline memorySpaceList_t queryMemorySpacesImpl() override
  {
    // Getting total system memory
    _totalSystemMem = Sequential::getTotalSystemMemory();

    // Only a single memory space is created
    return memorySpaceList_t({0});
  }

  __USED__ inline std::unique_ptr<ProcessingUnit> createProcessingUnitImpl(computeResourceId_t resource) const override
  {
    return std::move(std::make_unique<Process>(resource));
  }

  __USED__ inline void memcpyImpl(memorySlotId_t destination, const size_t dst_offset, const memorySlotId_t source, const size_t src_offset, const size_t size) override
  {
    // Getting pointer for the corresponding slots
    const auto srcSlot = _memorySlotMap.at(source);
    const auto dstSlot = _memorySlotMap.at(destination);

    // Getting slot pointers
    const auto srcPtr = srcSlot.pointer;
    const auto dstPtr = dstSlot.pointer;

    // Calculating actual offsets
    const auto actualSrcPtr = (void *)((uint8_t *)srcPtr + src_offset);
    const auto actualDstPtr = (void *)((uint8_t *)dstPtr + dst_offset);

    // Running memcpy now
    std::memcpy(actualDstPtr, actualSrcPtr, size);

    // Increasing message received/sent counters for memory slots
    _memorySlotMap[source].messagesSent++;
    _memorySlotMap[destination].messagesRecv++;
  }

  /**
   * Queries the backend to update the internal state of the memory slot.
   * One main use case of this function is to update the number of messages received and sent to/from this slot.
   * This is a non-blocking, non-collective function.
   *
   * \param[in] memorySlotId Identifier of the memory slot to query for updates.
   */
  __USED__ inline void queryMemorySlotUpdatesImpl(const memorySlotId_t memorySlotId) override
  {
   // This function should check and update the abstract class for completed memcpy operations
  }

  /**
   * Implementation of the fence operation for the sequential backend. In this case, nothing needs to be done, as
   * the system's memcpy operation is synchronous. This means that it's mere execution (whether immediate or deferred)
   * ensures its completion.
   */
  __USED__ inline void fenceImpl()
  {
  }

  /**
   * Allocates memory in the current memory space (whole system)
   *
   * \param[in] memorySpace Memory space in which to perform the allocation.
   * \param[in] size Size of the memory slot to create
   * \param[in] memSlotId The identifier of the new memory slot
   */
  __USED__ inline void* allocateMemorySlotImpl(const memorySpaceId_t memorySpace, const size_t size, const memorySlotId_t memSlotId) override
  {
    // Atempting to allocate the new memory slot
    auto ptr = malloc(size);

    // Check whether it was successful
    if (ptr == NULL) HICR_THROW_RUNTIME("Could not allocate memory of size %lu", size);

    // Now returning pointer
    return ptr;
  }

  /**
   * Associates a pointer allocated somewhere else and creates a memory slot with it
   * \param[in] addr Address in local memory that will be represented by the slot
   * \param[in] size Size of the memory slot to create
   * \param[in] The identifier for the new memory slot
   */
  __USED__ inline void registerMemorySlotImpl(void *const addr, const size_t size, const memorySlotId_t memSlotId) override
  {
    // Nothing to do here for this backend
  }

  /**
   * De-registers a memory slot previously registered
   *
   * \param[in] memorySlotId Identifier of the memory slot to deregister.
   */
  __USED__ inline void deregisterMemorySlotImpl(memorySlotId_t memorySlotId) override
  {
   // Nothing to do here for this backend
  }

  /**
   * Frees up a memory slot reserved from this memory space
   *
   * \param[in] memorySlotId Identifier of the memory slot to free up. It becomes unusable after freeing.
   */
  __USED__ inline void freeMemorySlotImpl(memorySlotId_t memorySlotId) override
  {
    const auto &memSlot = _memorySlotMap.at(memorySlotId);

    if (memSlot.pointer == NULL) HICR_THROW_RUNTIME("Invalid memory slot(s) (%lu) provided. It either does not exit or represents a NULL pointer.", memorySlotId);

    free(memSlot.pointer);
  }

  /**
   * Checks whether the memory slot id exists and is valid.
   *
   * In this backend, this means that the memory slot was either allocated or created and it contains a non-NULL pointer.
   *
   * \param[in] memorySlotId Identifier of the slot to check
   * \return True, if the referenced memory slot exists and is valid; false, otherwise
   */
  __USED__ bool isMemorySlotValidImpl(const memorySlotId_t memorySlotId) const override
  {
    // Getting pointer for the corresponding slot
    const auto &slot = _memorySlotMap.at(memorySlotId);

    // If it is NULL, it means it was never created
    if (slot.pointer == NULL) return false;

    // Otherwise it is ok
    return true;
  }

};

} // namespace sequential
} // namespace backend
} // namespace HiCR
