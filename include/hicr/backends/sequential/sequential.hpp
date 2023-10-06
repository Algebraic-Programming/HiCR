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

#include <hicr/common/definitions.hpp>
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
  public:

  /**
   * Common definition of a collection of memory slots
   */
  typedef std::map<tag_t, size_t> fenceCountTagMap_t;

  /**
   * Constructor for the sequential backend.
   *
   * \param[in] fenceCount Specifies how many times a fence has to be called for it to release callers
   */
  Sequential(const size_t fenceCount = 1) : Backend(), _fenceCount(fenceCount) {}
  ~Sequential() = default;

  private:

  /**
   * Specifies how many times a fence has to be called for it to release callers
   */
  const size_t _fenceCount;

  /**
   * Counter for calls to fence, filtered per tag
   */
  fenceCountTagMap_t _fenceCountTagMap;

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

  __USED__ inline void memcpyImpl(MemorySlot *destination, const size_t dst_offset, MemorySlot *source, const size_t src_offset, const size_t size) override
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

  /**
   * Queries the backend to update the internal state of the memory slot.
   * One main use case of this function is to update the number of messages received and sent to/from this slot.
   * This is a non-blocking, non-collective function.
   *
   * \param[in] memorySlot Memory slot to query for updates.
   */
  __USED__ inline void queryMemorySlotUpdatesImpl(const MemorySlot *memorySlot) override
  {
  }

  /**
   * Implementation of the fence operation for the sequential backend. In this case, nothing needs to be done, as
   * the memcpy operation is synchronous. This means that it's mere execution (whether immediate or deferred)
   * ensures its completion.
   */
  __USED__ inline void fenceImpl(const tag_t tag, const globalKeyToMemorySlotArrayMap_t &globalSlots) override
  {
    // Increasing the counter for the fence corresponding to the tag
    _fenceCountTagMap[tag]++;

    // Until we reached the required count, wait on it
    while (_fenceCountTagMap[tag] % _fenceCount != 0)
      ;
  }

  /**
   * Allocates memory in the current memory space (whole system)
   *
   * \param[in] memorySpace Memory space in which to perform the allocation.
   * \param[in] size Size of the memory slot to create
   * \returns The pointer of the newly allocated memory slot
   */
  __USED__ inline void *allocateLocalMemorySlotImpl(const memorySpaceId_t memorySpace, const size_t size) override
  {
    // Atempting to allocate the new memory slot
    auto ptr = malloc(size);

    // Check whether it was successful
    if (ptr == NULL) HICR_THROW_RUNTIME("Could not allocate memory of size %lu", size);

    // Now returning pointer
    return ptr;
  }

  /**
   * Associates a pointer locally-allocated manually and creates a local memory slot with it
   * \param[in] memorySlot The new local memory slot to register
   */
  __USED__ inline void registerLocalMemorySlotImpl(const MemorySlot *memorySlot) override
  {
    // Nothing to do here for this backend
  }

  /**
   * De-registers a memory slot previously registered
   *
   * \param[in] memorySlot Memory slot to deregister.
   */
  __USED__ inline void deregisterLocalMemorySlotImpl(MemorySlot *memorySlot) override
  {
    // Nothing to do here for this backend
  }

  __USED__ inline void deregisterGlobalMemorySlotImpl(MemorySlot *memorySlot) override
  {
    // Nothing to do here
  }

  /**
   * Exchanges memory slots among different local instances of HiCR to enable global (remote) communication
   *
   * \param[in] tag Identifies a particular subset of global memory slots
   * \param[in] memorySlots Array of local memory slots to make globally accessible
   */
  __USED__ inline void exchangeGlobalMemorySlots(const tag_t tag, const std::vector<globalKeyMemorySlotPair_t> &memorySlots) override
  {
    // Simply adding local memory slots to the global map
    for (const auto &entry : memorySlots)
    {
      const auto key = entry.first;
      const auto memorySlot = entry.second;
      registerGlobalMemorySlot(tag, key, memorySlot->getPointer(), memorySlot->getSize());
    }
  }

  /**
   * Backend-internal implementation of the freeLocalMemorySlot function
   *
   * \param[in] memorySlot Local memory slot to free up. It becomes unusable after freeing.
   */
  __USED__ inline void freeLocalMemorySlotImpl(MemorySlot *memorySlot) override
  {
    if (memorySlot->getPointer() == NULL) HICR_THROW_RUNTIME("Invalid memory slot(s) (%lu) provided. It either does not exit or represents a NULL pointer.", memorySlot->getId());

    free(memorySlot->getPointer());
  }

  /**
   * Backend-internal implementation of the isMemorySlotValid function
   *
   * \param[in] memorySlot Memory slot to check
   * \return True, if the referenced memory slot exists and is valid; false, otherwise
   */
  __USED__ bool isMemorySlotValidImpl(const MemorySlot *memorySlot) const override
  {
    // If it is NULL, it means it was never created
    if (memorySlot->getPointer() == NULL) return false;

    // Otherwise it is ok
    return true;
  }
};

} // namespace sequential
} // namespace backend
} // namespace HiCR
