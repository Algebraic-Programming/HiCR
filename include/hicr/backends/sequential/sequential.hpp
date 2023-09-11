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
 * Internal representation of a memory slot for the sequential backend
 */
struct memorySlotStruct_t
{
  /**
   * Pointer to the local memory address containing this slot
   */
  void *pointer;

  /**
   * Size of the memory slot
   */
  size_t size;
};

/**
 * Implementation of the HiCR Sequential backend
 *
 * This backend is very useful for testing other HiCR modules in isolation (unit tests) without involving the use of threading, which might incur side-effects
 */
class Sequential final : public Backend
{
  private:

  /**
   * Currently available tag id to be assigned. It should increment as each tag is assigned
   */
  memorySlotId_t _currentTagId = 0;

  /**
   * Thread-safe map that stores all allocated or created memory slots associated to this backend
   */
  parallelHashMap_t<memorySlotId_t, memorySlotStruct_t> _slotMap;

  /**
   * list of deffered function calls in non-blocking data moves, which
   * complete in the wait call
   */
  std::multimap<uint64_t, std::future<void>> deferredFuncs;

  /**
   * This function returns the system physical memory size, which is what matters for a sequential program
   *
   * This is adapted from https://stackoverflow.com/a/2513561
   */
  size_t getTotalSystemMemory()
  {
    size_t pages = sysconf(_SC_PHYS_PAGES);
    size_t page_size = sysconf(_SC_PAGE_SIZE);
    return pages * page_size;
  }

  public:

  /**
   * Pthread implementation of the Backend queryResources() function. This will add one resource object per found Thread / Processing Unit (PU)
   */
  __USED__ inline void queryResources() override
  {
    // Only a single processing unit is created
    _computeResourceList = computeResourceList_t({0});

    // Only a single memory space is created
    _memorySpaceList = memorySpaceList_t({0});
  }

  __USED__ inline std::unique_ptr<ProcessingUnit> createProcessingUnit(computeResourceId_t resource) const override
  {
    return std::move(std::make_unique<Process>(resource));
  }

  __USED__ inline void memcpy(memorySlotId_t destination, const size_t dst_offset, const memorySlotId_t source, const size_t src_offset, const size_t size, const tagId_t &tag) override
  {
    std::function<void(void *, const void *, size_t)> f = [](void *dst, const void *src, size_t size)
    { std::memcpy(dst, src, size); };
    const auto srcSlot = _slotMap.at(source);
    const auto dstSlot = _slotMap.at(destination);
    std::future<void> fut = std::async(std::launch::deferred, f, dstSlot.pointer, srcSlot.pointer, size);
    deferredFuncs.insert(std::make_pair(tag, std::move(fut)));
  }

  /**
   * Waits for the completion of one or more pending operation(s) associated with a given tag
   *
   * \param[in] tag Identifier of the operation(s) to be waited  upon
   *
   * TO-DO: This all should be threading safe
   */
  __USED__ inline void fence(const uint64_t tag) override
  {
    auto range = deferredFuncs.equal_range(tag);
    for (auto i = range.first; i != range.second; ++i)
    {
      auto f = std::move(i->second);
      f.wait();
    }
  }

  /**
   * Allocates memory in the current memory space (whole system)
   *
   * \param[in] memorySpace Memory space in which to perform the allocation.
   * \param[in] size Size of the memory slot to create
   * \return A newly allocated memory slot in this memory space
   *
   * TO-DO: This all should be threading safe
   */
  __USED__ inline memorySlotId_t allocateMemorySlot(const memorySpaceId_t memorySpace, const size_t size) override
  {
    auto ptr = malloc(size);
    auto tag = _currentTagId++;
    _slotMap[tag] = memorySlotStruct_t{.pointer = ptr, .size = size};
    return tag;
  }

  /**
   * Associates a pointer allocated somewhere else and creates a memory slot with it
   * \param[in] addr Address in local memory that will be represented by the slot
   * \param[in] size Size of the memory slot to create
   * \return The id of the memory slot that represents the given pointer
   */
  __USED__ inline memorySlotId_t createMemorySlot(void *const addr, const size_t size) override
  {
    auto tag = _currentTagId++;
    _slotMap[tag] = memorySlotStruct_t{.pointer = addr, .size = size};
    return tag;
  }

  /**
   * Frees up a memory slot reserved from this memory space
   *
   * \param[in] memorySlotId Identifier of the memory slot to free up. It becomes unusable after freeing.
   */
  __USED__ inline void freeMemorySlot(memorySlotId_t memorySlotId)
  {
    const auto slot = _slotMap.at(memorySlotId);
    free(slot.pointer);
    _slotMap.erase(memorySlotId);
  }

  /**
   * Obtains the local pointer from a given memory slot.
   *
   * \param[in] memorySlotId Identifier of the slot from where to source the pointer.
   * \return The local memory pointer, if applicable. NULL, otherwise.
   */
  __USED__ inline void *getMemorySlotLocalPointer(const memorySlotId_t memorySlotId) const override
  {
    return _slotMap.at(memorySlotId).pointer;
  }

  /**
   * Obtains the size of the memory slot
   *
   * \param[in] memorySlotId Identifier of the slot from where to source the size.
   * \return The non-negative size of the memory slot, if applicable. Zero, otherwise.
   */
  __USED__ inline size_t getMemorySlotSize(const memorySlotId_t memorySlotId) const override
  {
    return _slotMap.at(memorySlotId).size;
  }
};

} // namespace sequential
} // namespace backend
} // namespace HiCR
