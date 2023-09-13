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
#include <memory>

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
  memorySlotId_t _currentMemorySlotId = 0;

  /**
   * Thread-safe map that stores all allocated or created memory slots associated to this backend
   */
  parallelHashMap_t<memorySlotId_t, memorySlotStruct_t> _memorySlotMap;

  /**
  * Thread-safe map that stores all detected memory spaces HWLoC objects associated to this backend
  */
 parallelHashMap_t<memorySpaceId_t, hwloc_obj_t> _memorySpaceMap;

  /**
   * Local processor and memory hierarchy topology, as detected by Hwloc
   */
  hwloc_topology_t _topology;

  /**
   * Pthread implementation of the Backend queryResources() function. This will add one compute resource object per Thread / Processing Unit (PU) found
   */
  __USED__ inline computeResourceList_t queryComputeResourcesImpl() override
  {
    // Loading topology
    hwloc_topology_load(_topology);

    // New compute resource list to return
    computeResourceList_t computeResourceList;

    // Creating compute resource list, based on the  processing units (hyperthreads) observed by HWLoc
    std::vector<int> threadPUs;
    getThreadPUs(_topology, hwloc_get_root_obj(_topology), 0, threadPUs);
    computeResourceList.insert(threadPUs.begin(), threadPUs.end());

    // Returning new compute resource list
    return computeResourceList;
  }


  /**
   * Pthread implementation of the Backend queryResources() function. This will add one memory space object per NUMA domain found
   */
  __USED__ inline memorySpaceList_t queryMemorySpacesImpl() override
  {
    // Loading topology
    hwloc_topology_load(_topology);

    // Clearing existing memory space map
    _memorySpaceMap.clear();

    // New memory space list to return
    memorySpaceList_t memorySpaceList;

    // Ask hwloc about number of NUMA nodes and add as many memory spaces as NUMA domains
    auto n = hwloc_get_nbobjs_by_type(_topology, HWLOC_OBJ_NUMANODE);
    for (int i = 0; i < n; i++)
    {
     // Storing reference to the HWLoc object for future reference
     _memorySpaceMap[i] = hwloc_get_obj_by_type(_topology, HWLOC_OBJ_NUMANODE, i);

     // Storing new memory space
     memorySpaceList.insert(i);
    }

    // Returning new memory space list
    return memorySpaceList;
  }

  __USED__ inline std::unique_ptr<ProcessingUnit> createProcessingUnitImpl(computeResourceId_t resource) const override
  {
    return std::move(std::make_unique<Thread>(resource));
  }

  __USED__ inline deferredFunction_t memcpyImpl(memorySlotId_t destination, const size_t dst_offset, const memorySlotId_t source, const size_t src_offset, const size_t size, const tagId_t &tag) override
  {
    // Getting pointer for the corresponding slots
    const auto srcSlot = _memorySlotMap.at(source);
    const auto dstSlot = _memorySlotMap.at(destination);

    // Getting slot pointers
    const auto srcPtr = srcSlot.pointer;
    const auto dstPtr = dstSlot.pointer;

    // Calculating actual offsets
    const auto actualSrcPtr = (void*)((uint8_t*)srcPtr + src_offset);
    const auto actualDstPtr = (void*)((uint8_t*)dstPtr + dst_offset);

    // Creating function that satisfies the request (memcpy)
    return [actualDstPtr, actualSrcPtr, size]() { std::memcpy(actualDstPtr, actualSrcPtr, size); };
  }


  public:

  /**
   * The constructor is employed to reserve memory required for hwloc
   */
  SharedMemory() : Backend()
  {
   // Reserving memory for hwloc
   hwloc_topology_init(&_topology);
  }

  /**
   * The constructor is employed to free memory required for hwloc
   */
  ~SharedMemory()
  {
   // Freeing Reserved memory for hwloc
   hwloc_topology_destroy(_topology);
  }

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
    auto maxSize = getMemorySpaceSize(memorySpace);
    if (size > maxSize) HICR_THROW_LOGIC("Attempting to allocate more memory (%lu) than available in the memory space (%lu)", size, maxSize);

    // Recovering HWLoc object corresponding to this memory space
    hwloc_obj_t obj = _memorySpaceMap.at(memorySpace);

    // Allocating memory in the reqested memory space
    auto ptr = hwloc_alloc_membind(_topology, size, obj->nodeset, HWLOC_MEMBIND_BIND, 0);

    // Error checking
    if (ptr == NULL) HICR_THROW_LOGIC("Could not allocate memory (size %lu) in the requested memory space (%lu)", size, memorySpace);

    // Incrementing memory slot id to prevent index re-use
    auto slotId = _currentMemorySlotId++;

    // Assinging new entry in the memory slot map
    _memorySlotMap[slotId] = memorySlotStruct_t{.pointer = ptr, .size = size};

    // Return the id of the just created slot
    return slotId;
  }

  /**
   * Associates a pointer allocated somewhere else and creates a memory slot with it
   * \param[in] addr Address in local memory that will be represented by the slot
   * \param[in] size Size of the memory slot to create
   * \return The id of the memory slot that represents the given pointer
   */
  __USED__ inline memorySlotId_t createMemorySlot(void *const addr, const size_t size) override
  {
   // Incrementing memory slot id to prevent index re-use
   auto slotId = _currentMemorySlotId++;

   // Inserting passed address into the memory slot map
    _memorySlotMap[slotId] = memorySlotStruct_t{.pointer = addr, .size = size};

    // Return the id of the just created slot
    return slotId;
  }

  /**
   * Frees up a memory slot reserved from this memory space
   *
   * \param[in] memorySlotId Identifier of the memory slot to free up. It becomes unusable after freeing.
   */
  __USED__ inline void freeMemorySlot(memorySlotId_t memorySlotId)
  {
    // Getting memory slot entry from the map
    const auto& slot = _memorySlotMap.at(memorySlotId);

    // Making sure the memory slot exists and is not null
    if (slot.pointer == NULL) HICR_THROW_RUNTIME("Invalid memory slot(s) (%lu) provided. It either does not exist or represents a NULL pointer.", memorySlotId);

    // Freeing memory slot
    auto status = hwloc_free(_topology, slot.pointer, slot.size);

    // Error checking
    if (status != 0) HICR_THROW_RUNTIME("Could not free memory slot (%lu), hwloc returned status: %u.", memorySlotId, status);

    // Erasing memory slot from the map
    _memorySlotMap.erase(memorySlotId);
  }

  /**
   * Obtains the local pointer from a given memory slot.
   *
   * \param[in] memorySlotId Identifier of the slot from where to source the pointer.
   * \return The local memory pointer, if applicable. NULL, otherwise.
   */
  __USED__ inline void *getMemorySlotLocalPointer(const memorySlotId_t memorySlotId) const override
  {
    return _memorySlotMap.at(memorySlotId).pointer;
  }

  /**
   * Obtains the size of the memory slot
   *
   * \param[in] memorySlotId Identifier of the slot from where to source the size.
   * \return The non-negative size of the memory slot, if applicable. Zero, otherwise.
   */
  __USED__ inline size_t getMemorySlotSize(const memorySlotId_t memorySlotId) const override
  {
    return _memorySlotMap.at(memorySlotId).size;
  }

  /**
   * This function returns the available allocatable size in the NUMA domain represented by the given memory space
   *
   * @param[in] memorySpace The NUMA domain to query
   * @return The allocatable size within that NUMA domain
   */
  __USED__ inline size_t getMemorySpaceSizeImpl(const memorySpaceId_t memorySpace) const override
  {
     // Recovering HWLoc object corresponding to this memory space
    const auto obj = _memorySpaceMap.at(memorySpace);

    // Returning entry corresponding to the memory size
    return obj->attr->cache.size;
  }

  /**
   * Checks whether the memory slot id exists and is valid.
   *
   * In this backend, this means that the memory slot was either allocated or created and it contains a non-NULL pointer.
   *
   * \param[in] memorySlotId Identifier of the slot to check
   * \return True, if the referenced memory slot exists and is valid; false, otherwise
   */
  __USED__ bool isMemorySlotValid(const memorySlotId_t memorySlotId) const override
  {
   // Getting pointer for the corresponding slot
   const auto slot = _memorySlotMap.at(memorySlotId);

   // If it is NULL, it means it was never created
   if (slot.pointer == NULL) return false;

   // Otherwise it is ok
   return true;
  }
};

} // namespace sharedMemory
} // namespace backend
} // namespace HiCR
