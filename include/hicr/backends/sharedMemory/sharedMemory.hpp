/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file sharedMemory.hpp
 * @brief This is a minimal backend for shared memory multi-core support based on HWLoc and Pthreads
 * @author S. M. Martin
 * @date 14/8/2023
 */

#pragma once

#include <cstring>
#include <errno.h>
#include <future>
#include <memory>
#include <stdio.h>

#include "hwloc.h"

#include <hicr/backend.hpp>
#include <hicr/backends/sharedMemory/thread.hpp>

namespace HiCR
{

namespace backend
{

namespace sharedMemory
{

/**
 * Internal representation of a memory slot for the shared memory backend
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
 * Implementation of the SharedMemory/HWloc-based HiCR Shared Memory Backend.
 *
 * It detects and returns the processing units and memory spaces detected by the HWLoc library, and instantiates the former as SharedMemory and the latter as MemorySpace descriptors. It also detects their connectivity.
 */
class SharedMemory final : public Backend
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
   * Local processor and memory hierarchy topology, as detected by Hwloc
   */
  hwloc_topology_t _topology;

  public:

  /**
   * Uses HWloc to recursively (tree-like) identify the system's basic processing units (PUs)
   *
   * \param[in] topology An HWLoc topology object, already initialized
   * \param[in] obj The root HWLoc object for the start of the exploration tree at every recursion level
   * \param[in] depth Stores the current exploration depth level, necessary to return only the processing units at the leaf level
   * \param[out] threadPUs Storage for the found procesing units
   */
  __USED__ inline static void getThreadPUs(hwloc_topology_t topology, hwloc_obj_t obj, int depth, std::vector<int> &threadPUs)
  {
    if (obj->arity == 0) threadPUs.push_back(obj->os_index);
    for (unsigned int i = 0; i < obj->arity; i++) getThreadPUs(topology, obj->children[i], depth + 1, threadPUs);
  }

  /**
   * Pthread implementation of the Backend queryResources() function. This will add one resource object per found Thread / Processing Unit (PU)
   */
  __USED__ inline void queryResources() override
  {
    hwloc_topology_init(&_topology);
    hwloc_topology_load(_topology);

    // Creating compute resource list, based on the  processing units (hyperthreads) observed by HWLoc
    std::vector<int> threadPUs;
    getThreadPUs(_topology, hwloc_get_root_obj(_topology), 0, threadPUs);
    _computeResourceList.assign(threadPUs.begin(), threadPUs.end());

    /* Ask hwloc about number of NUMA nodes
     * and add as many memory spaces as NUMA domains
     */
    _memorySpaceList.clear();
    auto n = hwloc_get_nbobjs_by_type(_topology, HWLOC_OBJ_NUMANODE);
    for (int i = 0; i < n; i++) _memorySpaceList.push_back(i);
  }

  __USED__ inline std::unique_ptr<ProcessingUnit> createProcessingUnit(computeResourceId_t resource) const override
  {
    return std::move(std::make_unique<Thread>(resource));
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
   * Allocates memory in the current memory space (NUMA domain)
   *
   * \param[in] memorySpace Memory space in which to perform the allocation.
   * \param[in] size Size of the memory slot to create
   * \return A newly allocated memory slot in this memory space
   *
   * TO-DO: This all should be threading safe
   */
  __USED__ inline memorySlotId_t allocateMemorySlot(const memorySpaceId_t memorySpace, const size_t size) override
  {
    hwloc_obj_t obj = hwloc_get_obj_by_type(_topology, HWLOC_OBJ_NUMANODE, memorySpace);
    auto ptr = hwloc_alloc_membind(_topology, size, obj->nodeset, HWLOC_MEMBIND_BIND, HWLOC_MEMBIND_BYNODESET);
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
    hwloc_free(_topology, slot.pointer, slot.size);
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

} // namespace sharedMemory
} // namespace backend
} // namespace HiCR
