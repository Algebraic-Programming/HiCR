/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file sharedMemory.hpp
 * @brief This is a minimal backend for shared memory system support based on HWLoc
 * @author S. M. Martin
 * @date 14/8/2023
 */

#pragma once

#include "hwloc.h"
#include <hicr/backends/memoryManager.hpp>

namespace HiCR
{

namespace backend
{

namespace sharedMemory
{

/**
 * Implementation of the SharedMemory/HWloc-based HiCR Shared Memory Backend.
 */
class MemoryManager final : public backend::MemoryManager
{
  public:

  /**
   * The constructor is employed to reserve memory required for hwloc
   *
   * \param[in] fenceCount Specifies how many times a fence has to be called for it to release callers
   */
  MemoryManager(const hwloc_topology_t * topology) : backend::MemoryManager(), _topology { topology } { }

  /**
   * The constructor is employed to free memory required for hwloc
   */
  ~MemoryManager() = default;

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
   * Specifies the biding support requested by the user. It should be by default strictly binding to follow HiCR's design, but can be relaxed upon request, when binding does not matter or a first touch policy is followed
   */
  binding_type _hwlocBindingRequested = binding_type::strict_binding;

  /**
   * Thread-safe map that stores all allocated or created memory slots associated to this backend
   */
  parallelHashMap_t<void *, binding_type> _memorySlotBindingMap;

  /**
   * Thread-safe map that stores all detected memory spaces HWLoC objects associated to this backend
   */
  parallelHashMap_t<memorySpaceId_t, memorySpace_t> _memorySpaceMap;

  /**
   * Local processor and memory hierarchy topology, as detected by Hwloc
   */
  const hwloc_topology_t* const _topology;

  /**
   * Backend-internal implementation of the isMemorySlotValid function
   *
   * \param[in] memorySlot Memory slot to check
   * \return True, if the referenced memory slot exists and is valid; false, otherwise
   */
  __USED__ bool isMemorySlotValidImpl(const MemorySlot *memorySlot) const override
  {
    if (memorySlot->getPointer() == NULL) return false;

    // Otherwise it is ok
    return true;
  }

  /**
   * Pthread implementation of the Backend queryResources() function. This will add one memory space object per NUMA domain found
   */
  __USED__ inline memorySpaceList_t queryMemorySpacesImpl() override
  {
    // Loading topology
    hwloc_topology_load(*_topology);

    // Clearing existing memory space map
    _memorySpaceMap.clear();

    // New memory space list to return
    memorySpaceList_t memorySpaceList;

    // Ask hwloc about number of NUMA nodes and add as many memory spaces as NUMA domains
    auto n = hwloc_get_nbobjs_by_type(*_topology, HWLOC_OBJ_NUMANODE);
    for (int i = 0; i < n; i++)
    {
      // Getting HWLoc object related to this NUMA domain
      auto obj = hwloc_get_obj_by_type(*_topology, HWLOC_OBJ_NUMANODE, i);

      // Checking whther bound memory allocation and freeing is supported
      binding_type bindingSupport = strict_non_binding;
      size_t size = 1024;
      auto ptr = hwloc_alloc_membind(*_topology, size, obj->nodeset, HWLOC_MEMBIND_DEFAULT, HWLOC_MEMBIND_BYNODESET | HWLOC_MEMBIND_STRICT);
      if (ptr != NULL)
      {
        // Attempting to free with hwloc
        auto status = hwloc_free(*_topology, ptr, size);

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

  /**
   * Backend-internal implementation of the queryLocalMemorySlot function
   *
   * \param[in] memorySpaceId Memory space to allocate memory in
   * \param[in] size Size of the memory slot to create
   * \return The internal pointer associated to the local memory slot
   */
  __USED__ inline void *allocateLocalMemorySlotImpl(const memorySpaceId_t memorySpaceId, const size_t size) override
  {
    // Recovering memory space from the map
    auto &memSpace = _memorySpaceMap.at(memorySpaceId);

    // Checking whether the operation requested is supported by the HWLoc on this memory space
    if (_hwlocBindingRequested > memSpace.bindingSupport) HICR_THROW_LOGIC("Requesting an allocation binding support level (%u) not supported by the operating system (HWLoc max support: %u)", _hwlocBindingRequested, memSpace.bindingSupport);

    // Allocating memory in the reqested memory space
    void *ptr = NULL;
    if (memSpace.bindingSupport == binding_type::strict_binding) ptr = hwloc_alloc_membind(*_topology, size, memSpace.obj->nodeset, HWLOC_MEMBIND_DEFAULT, HWLOC_MEMBIND_BYNODESET | HWLOC_MEMBIND_STRICT);
    if (memSpace.bindingSupport == binding_type::strict_non_binding) ptr = malloc(size);

    // Error checking
    if (ptr == NULL) HICR_THROW_LOGIC("Could not allocate memory (size %lu) in the requested memory space (%lu)", size, memorySpaceId);

    // Remembering binding support for the current memory slot
    _memorySlotBindingMap[ptr] = memSpace.bindingSupport;

    // Assinging new entry in the memory slot map
    return ptr;
  }

  /**
   * Backend-internal implementation of the registerLocalMemorySlot function
   *
   * \param[in] memorySlot The new local memory slot to register
   */
  __USED__ inline void registerLocalMemorySlotImpl(const MemorySlot *memorySlot) override
  {
    // Nothing to do here
  }

  /**
   * Backend-internal implementation of the deregisterMemorySlot function
   *
   * \param[in] memorySlot Memory slot to deregister.
   */
  __USED__ inline void deregisterLocalMemorySlotImpl(MemorySlot *const memorySlot) override
  {
    // Nothing to do here
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
  __USED__ inline void exchangeGlobalMemorySlotsImpl(const tag_t tag, const std::vector<globalKeyMemorySlotPair_t> &memorySlots) override
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
    // Getting memory slot info
    const auto memorySlotId = memorySlot->getId();
    const auto memorySlotPointer = memorySlot->getPointer();
    const auto memorySlotSize = memorySlot->getSize();

    // If using strict binding, use hwloc_free to properly unmap the memory binding
    if (_memorySlotBindingMap.at(memorySlotPointer) == binding_type::strict_binding)
    {
      // Freeing memory slot
      auto status = hwloc_free(*_topology, memorySlotPointer, memorySlotSize);

      // Error checking
      if (status != 0) HICR_THROW_RUNTIME("Could not free bound memory slot (%lu).", memorySlotId);
    }

    // If using strict non binding, use system's free
    if (_memorySlotBindingMap.at(memorySlotPointer) == binding_type::strict_non_binding)
    {
      free(memorySlotPointer);
    }

    // Erasing memory slot from the binding information map
    _memorySlotBindingMap.erase(memorySlotPointer);
  }

  /**
   * Backend-internal implementation of the queryMemorySlotUpdates function
   *
   * \param[in] memorySlot Memory slot to query updates for.
   */
  __USED__ inline void queryMemorySlotUpdatesImpl(const MemorySlot *memorySlot) override
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
