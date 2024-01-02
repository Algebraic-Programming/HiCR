/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file memoryManager.hpp
 * @brief This file implements the memory manager support for the shared memory backend
 * @author S. M. Martin
 * @date 14/8/2023
 */

#pragma once

#include "hwloc.h"
#include "pthread.h"
#include <backends/sharedMemory/L0/memorySpace.hpp>
#include <backends/sharedMemory/L0/localMemorySlot.hpp>
#include <backends/sharedMemory/L0/globalMemorySlot.hpp>
#include <hicr/L1/memoryManager.hpp>

namespace HiCR
{

namespace backend
{

namespace sharedMemory
{

namespace L1
{

/**
 * Implementation of the SharedMemory/HWloc-based HiCR Shared Memory Backend.
 */
class MemoryManager final : public HiCR::L1::MemoryManager
{
  public:

  /**
   * Constructor for the memory manager class for the shared memory backend
   *
   * \param[in] topology An HWloc topology object that can be used to query the available memory resources
   */
  MemoryManager(const hwloc_topology_t *topology) : HiCR::L1::MemoryManager(), _topology{topology} {}
  ~MemoryManager() = default;

  private:

  /**
   * Specifies the biding support requested by the user. It should be by default strictly binding to follow HiCR's design, but can be relaxed upon request, when binding does not matter or a first touch policy is followed
   */
  L0::LocalMemorySlot::binding_type _hwlocBindingRequested = L0::LocalMemorySlot::binding_type::strict_binding;

  /**
   * Local processor and memory hierarchy topology, as detected by Hwloc
   */
  const hwloc_topology_t *const _topology;

  /**
   * Backend-internal implementation of the queryLocalMemorySlot function
   *
   * \param[in] memorySpace Memory space to allocate memory in
   * \param[in] size Size of the memory slot to create
   * \return The internal pointer associated to the local memory slot
   */
  __USED__ inline HiCR::L0::LocalMemorySlot *allocateLocalMemorySlotImpl(HiCR::L0::MemorySpace *memorySpace, const size_t size) override
  {
    // Getting up-casted pointer for the MPI instance
    auto m = dynamic_cast<const L0::MemorySpace *>(memorySpace);

    // Checking whether the execution unit passed is compatible with this backend
    if (m == NULL) HICR_THROW_LOGIC("The passed memory space is not supported by this memory manager\n");

    // Getting binding type supported by the memory space
    const auto supportedBindingType = m->getSupportedBindingType();

    // Checking whether the operation requested is supported by the HWLoc on this memory space
    if (_hwlocBindingRequested > supportedBindingType) HICR_THROW_LOGIC("Requesting an allocation binding support level (%u) not supported by the operating system (HWLoc max support: %u)", _hwlocBindingRequested, supportedBindingType);

    // Getting memory space's HWLoc object to perform the operation with
    const auto hwlocObj = m->getHWLocObject();

    // Allocating memory in the reqested memory space
    void *ptr = NULL;
    if (supportedBindingType == L0::LocalMemorySlot::binding_type::strict_binding) ptr = hwloc_alloc_membind(*_topology, size, hwlocObj->nodeset, HWLOC_MEMBIND_DEFAULT, HWLOC_MEMBIND_BYNODESET | HWLOC_MEMBIND_STRICT);
    if (supportedBindingType == L0::LocalMemorySlot::binding_type::strict_non_binding) ptr = malloc(size);

    // Error checking
    if (ptr == NULL) HICR_THROW_RUNTIME("Could not allocate memory (size %lu) in the requested memory space", size);

    // Creating new memory slot object
    auto memorySlot = new L0::LocalMemorySlot(supportedBindingType, ptr, size, memorySpace);

    // Assinging new entry in the memory slot map
    return memorySlot;
  }

  /**
   * Backend-internal implementation of the registerLocalMemorySlot function
   *
   * \param[in] memorySpace The memory space to register the new memory slot in
   * \param[in] ptr Pointer to the local memory space
   * \param[in] size Size of the memory slot to register
   * \return A newly created memory slot
   */
  __USED__ inline HiCR::L0::LocalMemorySlot *registerLocalMemorySlotImpl(HiCR::L0::MemorySpace *memorySpace, void *const ptr, const size_t size) override
  {
    // Creating new memory slot object
    auto memorySlot = new L0::LocalMemorySlot(L0::LocalMemorySlot::binding_type::strict_non_binding, ptr, size, memorySpace);

    // Returning new memory slot pointer
    return memorySlot;
  }

  /**
   * Backend-internal implementation of the deregisterMemorySlot function
   *
   * \param[in] memorySlot Memory slot to deregister.
   */
  __USED__ inline void deregisterLocalMemorySlotImpl(HiCR::L0::LocalMemorySlot *const memorySlot) override
  {
    // Nothing to do here
  }

  /**
   * Backend-internal implementation of the freeLocalMemorySlot function
   *
   * \param[in] memorySlot Local memory slot to free up. It becomes unusable after freeing.
   */
  __USED__ inline void freeLocalMemorySlotImpl(HiCR::L0::LocalMemorySlot *memorySlot) override
  {
    // Getting up-casted pointer for the execution unit
    auto m = dynamic_cast<L0::LocalMemorySlot *>(memorySlot);

    // Checking whether the execution unit passed is compatible with this backend
    if (m == NULL) HICR_THROW_LOGIC("The passed memory slot is not supported by this backend\n");

    // Getting memory slot info
    const auto memorySlotBindingType = m->getBindingType();
    const auto memorySlotPointer = m->getPointer();
    const auto memorySlotSize = m->getSize();

    // If using strict binding, use hwloc_free to properly unmap the memory binding
    if (memorySlotBindingType == L0::LocalMemorySlot::binding_type::strict_binding)
    {
      // Freeing memory slot
      auto status = hwloc_free(*_topology, memorySlotPointer, memorySlotSize);

      // Error checking
      if (status != 0) HICR_THROW_RUNTIME("Could not free bound memory slot.");
    }

    // If using strict non binding, use system's free
    if (memorySlotBindingType == L0::LocalMemorySlot::binding_type::strict_non_binding)
    {
      free(memorySlotPointer);
    }
  }
};

} // namespace L1

} // namespace sharedMemory

} // namespace backend

} // namespace HiCR
