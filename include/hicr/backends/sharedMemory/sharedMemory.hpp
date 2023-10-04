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
   * The constructor is employed to reserve memory required for hwloc
   *
   * \param[in] fenceCount Specifies how many times a fence has to be called for it to release callers
   */
  SharedMemory(const size_t fenceCount = 1) : Backend()
  {
    // Reserving memory for hwloc
    hwloc_topology_init(&_topology);

    // Initializing barrier for fence operation
    pthread_barrier_init(&_fenceBarrier, NULL, fenceCount);
  }

  /**
   * The constructor is employed to free memory required for hwloc
   */
  ~SharedMemory()
  {
    // Freeing Reserved memory for hwloc
    hwloc_topology_destroy(_topology);

    // Freeing barrier memory
    pthread_barrier_destroy(&_fenceBarrier);
  }

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
   * Stores a barrier object to check on a fence operation
   */
  pthread_barrier_t _fenceBarrier;

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
   * Implementation of the fence operation for the shared memory backend. In this case, nothing needs to be done, as
   * the system's memcpy operation is synchronous. This means that it's mere execution (whether immediate or deferred)
   * ensures its completion.
   */
  __USED__ inline void fenceImpl(const tag_t tag, const globalKeyToMemorySlotArrayMap_t& globalSlots) override
  {
    pthread_barrier_wait(&_fenceBarrier);
  }

  /**
   * Specifies the biding support requested by the user. It should be by default strictly binding to follow HiCR's design, but can be relaxed upon request, when binding does not matter or a first touch policy is followed
   */
  binding_type _hwlocBindingRequested = binding_type::strict_binding;

  /**
   * Thread-safe map that stores all allocated or created memory slots associated to this backend
   */
  parallelHashMap_t<memorySlotId_t, binding_type> _memorySlotBindingMap;

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
  __USED__ bool isMemorySlotValidImpl(const MemorySlot* memorySlot) const override
  {
    if (memorySlot->getPointer() == NULL) return false;

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

  __USED__ inline void memcpyImpl(MemorySlot* destination, const size_t dst_offset, MemorySlot* source, const size_t src_offset, const size_t size) override
  {
    // Getting slot pointers
    const auto srcPtr = source->getPointer();
    const auto dstPtr = destination->getPointer();

    // Calculating actual offsets
    const auto actualSrcPtr = (void *)((uint8_t *)srcPtr + src_offset);
    const auto actualDstPtr = (void *)((uint8_t *)dstPtr + dst_offset);

    // Running the requested operation
    std::memcpy(actualDstPtr, actualSrcPtr, size);

    // Increasing message received/sent counters for memory slots
    source->increaseMessagesSent();
    destination->increaseMessagesRecv();
  }

  /**
   * Allocates a local memory slot in the current memory space (NUMA domain)
   *
   * \param[in] memorySpace Memory space in which to perform the allocation.
   * \param[in] size Size of the memory slot to create
   * \param[in] memSlotId The identifier for the new local memory slot
   *
   * \internal This all should be threading safe
   */
  __USED__ inline void *allocateLocalMemorySlotImpl(const memorySpaceId_t memorySpace, const size_t size, const memorySlotId_t memSlotId) override
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

    // Remembering binding support for the current memory slot
    _memorySlotBindingMap[memSlotId] = memSpace.bindingSupport;

    // Assinging new entry in the memory slot map
    return ptr;
  }

  /**
   * Associates a local pointer allocated somewhere else and creates a local memory slot with it
   * \param[in] addr Address in local memory that will be represented by the slot
   * \param[in] size Size of the memory slot to create
   * \param[in] memorySlotId The identifier for the local new memory slot
   */
  __USED__ inline void registerLocalMemorySlotImpl(void *const addr, const size_t size, const memorySlotId_t memorySlotId) override
  {
    // Nothing to do here
  }

  /**
   * De-registers a memory slot previously registered
   *
   * \param[in] memorySlotId Identifier of the memory slot to deregister.
   */
  __USED__ inline void deregisterLocalMemorySlotImpl(MemorySlot* const memorySlot) override
  {
    // Nothing to do here
  }

  /**
   * Exchanges memory slots among different local instances of HiCR to enable global (remote) communication
   *
   * \param[in] tag Identifies a particular subset of global memory slots
   */
  __USED__ inline void exchangeGlobalMemorySlots(const tag_t tag, const std::vector<globalKeyMemorySlotPair_t>& memorySlots)
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
   * Frees up a local memory slot reserved from this memory space
   *
   * \param[in] memorySlotId Identifier of the locally allocated memory slot to free up. It becomes unusable after freeing.
   */
  __USED__ inline void freeLocalMemorySlotImpl(MemorySlot* memorySlot) override
  {
    // Getting memory slot Id
    const auto memorySlotId = memorySlot->getId();

    // If using strict binding, use hwloc_free to properly unmap the memory binding
    if (_memorySlotBindingMap.at(memorySlotId) == binding_type::strict_binding)
    {
      // Freeing memory slot
      auto status = hwloc_free(_topology, memorySlot->getPointer(), memorySlot->getSize());

      // Error checking
      if (status != 0) HICR_THROW_RUNTIME("Could not free bound memory slot (%lu).", memorySlotId);
    }

    // If using strict non binding, use system's free
    if (_memorySlotBindingMap.at(memorySlotId) == binding_type::strict_non_binding)
    {
      free(memorySlot->getPointer());
    }

    // Erasing memory slot from the binding information map
    _memorySlotBindingMap.erase(memorySlotId);
  }

  /**
   * Queries the backend to update the internal state of the memory slot.
   * One main use case of this function is to update the number of messages received and sent to/from this slot.
   * This is a non-blocking, non-collective function.
   *
   * \param[in] memorySlotId Identifier of the memory slot to query for updates.
   */
  __USED__ inline void queryMemorySlotUpdatesImpl(const MemorySlot* memorySlot) override
  {
    // This function should check and update the abstract class for completed memcpy operations
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
