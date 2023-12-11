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
#include <unistd.h>
#include <hicr/common/definitions.hpp>
#include <hicr/common/exceptions.hpp>
#include <hicr/L1/memoryManager.hpp>

namespace HiCR
{

namespace backend
{

namespace sequential
{

namespace L1
{


/** 
 * This macro represents an identifier for the default system-wide memory space in this backend
 */
#define _BACKEND_SEQUENTIAL_DEFAULT_MEMORY_SPACE_ID 0

/**
 * Implementation of the SharedMemory/HWloc-based HiCR Shared Memory Backend.
 */
class MemoryManager final : public HiCR::L1::MemoryManager
{
  public:

  /**
   * The constructor is employed to create the barriers required to coordinate threads
   *
   * \param[in] fenceCount Specifies how many times a fence has to be called for it to release callers
   */
  MemoryManager(const size_t fenceCount = 1) : HiCR::L1::MemoryManager(), _fenceCount{fenceCount} {}
  ~MemoryManager() = default;

  /**
   * This function returns the system physical memory size, which is what matters for a sequential program
   *
   * This is adapted from https://stackoverflow.com/a/2513561
   *
   * \return the system physical memory size
   */
  __USED__ inline static size_t getTotalSystemMemory()
  {
    size_t pages = sysconf(_SC_PHYS_PAGES);
    size_t page_size = sysconf(_SC_PAGE_SIZE);
    return pages * page_size;
  }

  private:

  /**
   * This function returns the available allocatable size in the current system RAM
   *
   * @param[in] memorySpace Always zero, represents the system's RAM
   * @return The allocatable size within the system
   */
  __USED__ inline size_t getMemorySpaceSizeImpl(const memorySpaceId_t memorySpace) const override
  {
    if (memorySpace != _BACKEND_SEQUENTIAL_DEFAULT_MEMORY_SPACE_ID)
      HICR_THROW_RUNTIME("This backend does not support multiple memory spaces. Provided: %lu, Expected: %lu", memorySpace, (memorySpaceId_t)_BACKEND_SEQUENTIAL_DEFAULT_MEMORY_SPACE_ID);

    return getTotalSystemMemory();
  }

  /**
   * Sequential backend implementation that returns a single memory space representing the entire RAM host memory.
   */
  __USED__ inline memorySpaceList_t queryMemorySpacesImpl() override
  {
    // Only a single memory space is created
    return memorySpaceList_t({_BACKEND_SEQUENTIAL_DEFAULT_MEMORY_SPACE_ID});
  }

  /**
   * Queries the backend to update the internal state of the memory slot.
   * One main use case of this function is to update the number of messages received and sent to/from this slot.
   * This is a non-blocking, non-collective function.
   *
   * \param[in] memorySlot Memory slot to query for updates.
   */
  __USED__ inline void queryMemorySlotUpdatesImpl(HiCR::L0::MemorySlot *memorySlot) override
  {
  }

  /**
   * Allocates memory in the current memory space (whole system)
   *
   * \param[in] memorySpace Memory space in which to perform the allocation.
   * \param[in] size Size of the memory slot to create
   * \returns The pointer of the newly allocated memory slot
   */
  __USED__ inline HiCR::L0::MemorySlot *allocateLocalMemorySlotImpl(const memorySpaceId_t memorySpace, const size_t size) override
  {
    if (memorySpace != _BACKEND_SEQUENTIAL_DEFAULT_MEMORY_SPACE_ID) HICR_THROW_RUNTIME("This backend does not support multiple memory spaces. Provided: %lu, Expected: %lu", memorySpace, (memorySpaceId_t)_BACKEND_SEQUENTIAL_DEFAULT_MEMORY_SPACE_ID);

    // Atempting to allocate the new memory slot
    auto ptr = malloc(size);

    // Check whether it was successful
    if (ptr == NULL) HICR_THROW_RUNTIME("Could not allocate memory of size %lu", size);

    // Creating and returning new memory slot
    return registerLocalMemorySlotImpl(ptr, size);
  }

  /**
   * Associates a pointer locally-allocated manually and creates a local memory slot with it
   * \param[in] ptr Pointer to the local memory space
   * \param[in] size Size of the memory slot to register
   * \return A newly created memory slot
   */
  __USED__ inline HiCR::L0::MemorySlot *registerLocalMemorySlotImpl(void *const ptr, const size_t size) override
  {
    // Creating new memory slot object
    auto memorySlot = new HiCR::L0::MemorySlot(ptr, size);

    // Returning new memory slot pointer
    return memorySlot;
  }

  /**
   * De-registers a memory slot previously registered
   *
   * \param[in] memorySlot Memory slot to deregister.
   */
  __USED__ inline void deregisterLocalMemorySlotImpl(HiCR::L0::MemorySlot *memorySlot) override
  {
    // Nothing to do here for this backend
  }

  __USED__ inline void deregisterGlobalMemorySlotImpl(HiCR::L0::MemorySlot *memorySlot) override
  {
    // Nothing to do here
  }

  /**
   * Exchanges memory slots among different local instances of HiCR to enable global (remote) communication
   *
   * \param[in] tag Identifies a particular subset of global memory slots
   * \param[in] memorySlots Array of local memory slots to make globally accessible
   */
  __USED__ inline void exchangeGlobalMemorySlotsImpl(const HiCR::L0::MemorySlot::tag_t tag, const std::vector<globalKeyMemorySlotPair_t> &memorySlots) override
  {
    // Simply adding local memory slots to the global map
    for (const auto &entry : memorySlots)
    {
      // Getting global key
      auto globalKey = entry.first;

      // Getting local memory slot to promot
      auto memorySlot = entry.second;

      // Creating new memory slot
      auto globalMemorySlot = new HiCR::L0::MemorySlot(memorySlot->getPointer(), memorySlot->getSize(), tag, globalKey);

      // Registering memory slot
      registerGlobalMemorySlot(globalMemorySlot);
    }
  }

  /**
   * Backend-internal implementation of the freeLocalMemorySlot function
   *
   * \param[in] memorySlot Local memory slot to free up. It becomes unusable after freeing.
   */
  __USED__ inline void freeLocalMemorySlotImpl(HiCR::L0::MemorySlot *memorySlot) override
  {
    if (memorySlot->getPointer() == NULL) HICR_THROW_RUNTIME("Invalid memory slot(s) provided. It either does not exit or represents a NULL pointer.");

    free(memorySlot->getPointer());
  }

  /**
   * Specifies how many times a fence has to be called for it to release callers
   */
  const std::atomic<size_t> _fenceCount;

  /**
   * Common definition of a collection of memory slots
   */
  typedef parallelHashMap_t<HiCR::L0::MemorySlot::tag_t, size_t> fenceCountTagMap_t;

  /**
   * Counter for calls to fence, filtered per tag
   */
  fenceCountTagMap_t _fenceCountTagMap;

  /**
   * Implementation of the fence operation for the shared memory backend. In this case, nothing needs to be done, as
   * the system's memcpy operation is synchronous. This means that it's mere execution (whether immediate or deferred)
   * ensures its completion.
   */
  __USED__ inline void fenceImpl(const HiCR::L0::MemorySlot::tag_t tag) override
  {
    // Increasing the counter for the fence corresponding to the tag
    _fenceCountTagMap[tag]++;

    // Until we reached the required count, wait on it
    while (_fenceCountTagMap[tag] % _fenceCount != 0)
      ;
  }

  __USED__ inline void memcpyImpl(HiCR::L0::MemorySlot *destination, const size_t dst_offset, HiCR::L0::MemorySlot *source, const size_t src_offset, const size_t size) override
  {
    // Getting slot pointers
    const auto srcPtr = source->getPointer();
    const auto dstPtr = destination->getPointer();

    // Calculating actual offsets
    const auto actualSrcPtr = (void *)((uint8_t *)srcPtr + src_offset);
    const auto actualDstPtr = (void *)((uint8_t *)dstPtr + dst_offset);

    // Running memcpy now
    std::memcpy(actualDstPtr, actualSrcPtr, size);

    // Increasing message received/sent counters for memory slots
    source->increaseMessagesSent();
    destination->increaseMessagesRecv();
  }

  __USED__ inline bool acquireGlobalLockImpl(HiCR::L0::MemorySlot *memorySlot) override
  {
    // This function does not do anything because sequential applications
    // do not incur concurrency issues.

    return true;
  }

  __USED__ inline void releaseGlobalLockImpl(HiCR::L0::MemorySlot *memorySlot) override
  {
    // This function does not do anything because sequential applications
    // do not incur concurrency issues.
  }
};

} // namespace L1

} // namespace sequential

} // namespace backend

} // namespace HiCR
