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
 * Implementation of the SharedMemory/HWloc-based HiCR Shared Memory Backend.
 *
 * It detects and returns the processing units and memory spaces detected by the HWLoc library, and instantiates the former as SharedMemory and the latter as MemorySpace descriptors. It also detects their connectivity.
 */
class SharedMemory final : public Backend
{
  public:

  /**
   * Enumeration to determine whether HWLoc supports strict binding and what the user prefers (similar to MPI_Threading_level)
   */
  enum binding_type
  {
    /**
     * With strict binding, the memory is allocated strictly in the specified memory space
     */
    strict_binding = 1,

    /**
     * With strict non-binding, the memory is given by the system allocator. In this case, the binding is most likely setup by the first thread that touches the reserved pages (first touch policy)
     */
    strict_non_binding = 0
  };

  /**
   * Function to determine whether the memory space supports strictly bound memory allocations
   *
   * @param[in] memorySpace The memory space to check binding
   * @return The supported memory binding type by the memory space
   */
  __USED__ inline binding_type getSupportedBindingType(const memorySpaceId_t memorySpace) const
  {
    return _memorySpaceMap.at(memorySpace).bindingSupport;
  }

  /**
   * Function to set memory allocating binding type
   *
   * \param[in] type Specifies the desired binding type for future allocations
   */
  __USED__ inline void setRequestedBindingType(const binding_type type)
  {
    _hwlocBindingRequested = type;
  }

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

  private:

  /**
   * Structure representing a shared memory backend memory space
   */
  struct memorySpace_t
  {
    /**
     * HWloc object representing this memory space
     */
    hwloc_obj_t obj;

    /**
     * Stores whether it is possible to allocate bound memory in this memory space
     */
    binding_type bindingSupport;
  };

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

    /**
     * Store whether a bound memory allocation has performed
     */
    binding_type bindingType;
  };

  /**
   * Obtains the local pointer from a given memory slot.
   *
   * \param[in] memorySlotId Identifier of the slot from where to source the pointer.
   * \return The local memory pointer, if applicable. NULL, otherwise.
   */
  __USED__ inline void *getMemorySlotLocalPointerImpl(const memorySlotId_t memorySlotId) const override
  {
    return _memorySlotMap.at(memorySlotId).pointer;
  }

  /**
   * Obtains the size of the memory slot
   *
   * \param[in] memorySlotId Identifier of the slot from where to source the size.
   * \return The non-negative size of the memory slot, if applicable. Zero, otherwise.
   */
  __USED__ inline size_t getMemorySlotSizeImpl(const memorySlotId_t memorySlotId) const override
  {
    return _memorySlotMap.at(memorySlotId).size;
  }

  /**
   * Specifies the biding support requested by the user. It should be by default strictly binding to follow HiCR's design, but can be relaxed upon request, when binding does not matter or a first touch policy is followed
   */
  binding_type _hwlocBindingRequested = binding_type::strict_binding;

  /**
   * Thread-safe map that stores all allocated or created memory slots associated to this backend
   */
  parallelHashMap_t<memorySlotId_t, memorySlotStruct_t> _memorySlotMap;

  /**
   * Thread-safe map that stores all detected memory spaces HWLoC objects associated to this backend
   */
  parallelHashMap_t<memorySpaceId_t, memorySpace_t> _memorySpaceMap;

  /**
   * Local processor and memory hierarchy topology, as detected by Hwloc
   */
  hwloc_topology_t _topology;


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
    const auto slot = _memorySlotMap.at(memorySlotId);

    // If it is NULL, it means it was never created
    if (slot.pointer == NULL) return false;

    // Otherwise it is ok
    return true;
  }

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
      // Getting HWLoc object related to this NUMA domain
      auto obj = hwloc_get_obj_by_type(_topology, HWLOC_OBJ_NUMANODE, i);

      // Checking whther bound memory allocation and freeing is supported
      binding_type bindingSupport = strict_non_binding;
      size_t size = 1024;
      auto ptr = hwloc_alloc_membind(_topology, size, obj->nodeset, HWLOC_MEMBIND_DEFAULT, HWLOC_MEMBIND_BYNODESET | HWLOC_MEMBIND_STRICT);
      if (ptr != NULL)
      {
        // Attempting to free with hwloc
        auto status = hwloc_free(_topology, ptr, size);

        // Freeing was successful, then strict binding is supported
        if (status == 0) bindingSupport = strict_binding;
      }

      // Storing reference to the HWLoc object for future reference
      _memorySpaceMap[i] = memorySpace_t{.obj = obj, .bindingSupport = bindingSupport};

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
    const auto actualSrcPtr = (void *)((uint8_t *)srcPtr + src_offset);
    const auto actualDstPtr = (void *)((uint8_t *)dstPtr + dst_offset);

    // Creating function that satisfies the request (memcpy)
    return [actualDstPtr, actualSrcPtr, size]()
    { std::memcpy(actualDstPtr, actualSrcPtr, size); };
  }

  /**
   * Allocates memory in the current memory space (NUMA domain)
   *
   * \param[in] memorySpace Memory space in which to perform the allocation.
   * \param[in] size Size of the memory slot to create
   * \param[in] The identifier for the new memory slot
   *
   * TO-DO: This all should be threading safe
   */
  __USED__ inline void allocateMemorySlotImpl(const memorySpaceId_t memorySpace, const size_t size, const memorySlotId_t memSlotId) override
  {
    // Recovering memory space from the map
    auto &memSpace = _memorySpaceMap.at(memorySpace);

    // Checking whether the operation requested is supported by the HWLoc on this memory space
    if (_hwlocBindingRequested > memSpace.bindingSupport) HICR_THROW_LOGIC("Requesting an allocation binding support level (%u) not supported by the operating system (HWLoc max support: %u)", _hwlocBindingRequested, memSpace.bindingSupport);

    // Allocating memory in the reqested memory space
    void *ptr = NULL;
    if (memSpace.bindingSupport == binding_type::strict_binding) ptr = hwloc_alloc_membind(_topology, size, memSpace.obj->nodeset, HWLOC_MEMBIND_DEFAULT, HWLOC_MEMBIND_BYNODESET | HWLOC_MEMBIND_STRICT);
    if (memSpace.bindingSupport == binding_type::strict_non_binding) ptr = malloc(size);

    // Error checking
    if (ptr == NULL) HICR_THROW_LOGIC("Could not allocate memory (size %lu) in the requested memory space (%lu)", size, memorySpace);

    // Assinging new entry in the memory slot map
    _memorySlotMap[memSlotId] = memorySlotStruct_t{.pointer = ptr, .size = size, .bindingType = memSpace.bindingSupport};
  }

  /**
   * Associates a pointer allocated somewhere else and creates a memory slot with it
   * \param[in] addr Address in local memory that will be represented by the slot
   * \param[in] size Size of the memory slot to create
   * \param[in] The identifier for the new memory slot
   */
  __USED__ inline void registerMemorySlotImpl(void *const addr, const size_t size, const memorySlotId_t memSlotId) override
  {
    // Inserting passed address into the memory slot map
    _memorySlotMap[memSlotId] = memorySlotStruct_t{.pointer = addr, .size = size};
  }

  /**
   * De-registers a memory slot previously registered
   *
   * \param[in] memorySlotId Identifier of the memory slot to deregister.
   */
  __USED__ inline void deregisterMemorySlotImpl(memorySlotId_t memorySlotId) override
  {
   _memorySlotMap.erase(memorySlotId);
  }

  /**
   * Frees up a memory slot reserved from this memory space
   *
   * \param[in] memorySlotId Identifier of the memory slot to free up. It becomes unusable after freeing.
   */
  __USED__ inline void freeMemorySlotImpl(memorySlotId_t memorySlotId) override
  {
    // Getting memory slot entry from the map
    const auto &slot = _memorySlotMap.at(memorySlotId);

    // Making sure the memory slot exists and is not null
    if (slot.pointer == NULL) HICR_THROW_RUNTIME("Invalid memory slot(s) (%lu) provided. It either does not exist or represents a NULL pointer.", memorySlotId);

    // If using strict binding, use hwloc_free to properly unmap the memory binding
    if (slot.bindingType == binding_type::strict_binding)
    {
      // Freeing memory slot
      auto status = hwloc_free(_topology, slot.pointer, slot.size);

      // Error checking
      if (status != 0) HICR_THROW_RUNTIME("Could not free bound memory slot (%lu).", memorySlotId);
    }

    // If using strict non binding, use system's free
    if (slot.bindingType == binding_type::strict_non_binding)
    {
      free(slot.pointer);
    }

    // Erasing memory slot from the map
    _memorySlotMap.erase(memorySlotId);
  }

  /**
   * This function returns the available allocatable size in the NUMA domain represented by the given memory space
   *
   * @param[in] memorySpace The NUMA domain to query
   * @return The allocatable size within that NUMA domain
   */
  __USED__ inline size_t getMemorySpaceSizeImpl(const memorySpaceId_t memorySpace) const override
  {
    // Recovering memory space
    const auto memSpace = _memorySpaceMap.at(memorySpace);

    // Returning entry corresponding to the memory size
    return memSpace.obj->attr->cache.size;
  }
};

} // namespace sharedMemory
} // namespace backend
} // namespace HiCR
