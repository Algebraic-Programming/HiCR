/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file memoryManager.hpp
 * @brief This file implements the memory manager class for the HWLoc-based backend
 * @author S. M. Martin
 * @date 14/8/2023
 */

#pragma once

#include "hwloc.h"
#include "pthread.h"
#include <hicr/backends/hwloc/L0/memorySpace.hpp>
#include <hicr/backends/hwloc/L0/localMemorySlot.hpp>
#include <hicr/core/L1/memoryManager.hpp>

namespace HiCR::backend::hwloc::L1
{

/**
 * Implementation of the HWloc-based memory manager for allocation of memory in the host 
 */
class MemoryManager final : public HiCR::L1::MemoryManager
{
  public:

  /**
   * Constructor for the memory manager class for the shared memory backend
   *
   * \param[in] topology An HWloc topology object that can be used to query the available memory resources
   */
  MemoryManager(const hwloc_topology_t *topology)
    : HiCR::L1::MemoryManager(),
      _topology{topology}
  {}
  ~MemoryManager() override = default;

  /**
   * Sets the desired memory binding type before running an allocation attempt
   *
   * @param[in] type The new requested binding type to use
   */
  void setRequestedBindingType(const L0::LocalMemorySlot::binding_type type) { _hwlocBindingRequested = type; }

  /**
   * Gets the currently set desired memory binding type
   *
   * @return The currently requested binding type
   */
  [[nodiscard]] L0::LocalMemorySlot::binding_type getRequestedBindingType() const { return _hwlocBindingRequested; }

  /**
   * This function represents the default intializer for this backend
   *
   * @return A unique pointer to the newly instantiated backend class
   */
  __INLINE__ static std::unique_ptr<HiCR::L1::MemoryManager> createDefault()
  {
    // Creating HWloc topology object
    auto topology = new hwloc_topology_t;

    // Reserving memory for hwloc
    hwloc_topology_init(topology);

    // Initializing HWLoc-based host (CPU) topology manager
    return std::make_unique<HiCR::backend::hwloc::L1::MemoryManager>(topology);
  }

  private:

  /**
   * Specifies the biding support requested by the user.
   * This is set to relaxed binding by default, to try to accomplish the request but falling back to the non-binding on failure.
   */
  L0::LocalMemorySlot::binding_type _hwlocBindingRequested = L0::LocalMemorySlot::binding_type::relaxed_binding;

  /**
   * Local processor and memory hierarchy topology, as detected by Hwloc
   */
  const hwloc_topology_t *const _topology;

  __INLINE__ std::shared_ptr<HiCR::L0::LocalMemorySlot> allocateLocalMemorySlotImpl(std::shared_ptr<HiCR::L0::MemorySpace> memorySpace, const size_t size) override
  {
    // Getting up-casted pointer for the MPI instance
    auto m = dynamic_pointer_cast<L0::MemorySpace>(memorySpace);

    // Checking whether the execution unit passed is compatible with this backend
    if (m == nullptr) HICR_THROW_LOGIC("The passed memory space is not supported by this memory manager\n");

    // Getting binding type supported by the memory space
    const auto supportedBindingType = m->getSupportedBindingType();

    // Determining binding type to use
    L0::LocalMemorySlot::binding_type bindingTypeToUse = L0::LocalMemorySlot::binding_type::strict_binding;

    // Checking whether the operation requested is supported by the HWLoc on this memory space
    if (_hwlocBindingRequested == L0::LocalMemorySlot::binding_type::strict_binding) bindingTypeToUse = L0::LocalMemorySlot::binding_type::strict_binding;
    if (_hwlocBindingRequested == L0::LocalMemorySlot::binding_type::relaxed_binding && supportedBindingType == L0::LocalMemorySlot::binding_type::strict_binding)
      bindingTypeToUse = L0::LocalMemorySlot::binding_type::strict_binding;
    if (_hwlocBindingRequested == L0::LocalMemorySlot::binding_type::relaxed_binding && supportedBindingType == L0::LocalMemorySlot::binding_type::strict_non_binding)
      bindingTypeToUse = L0::LocalMemorySlot::binding_type::strict_non_binding;
    if (_hwlocBindingRequested == L0::LocalMemorySlot::binding_type::strict_non_binding) bindingTypeToUse = L0::LocalMemorySlot::binding_type::strict_non_binding;

    // Check for failure to provide strict binding
    if (_hwlocBindingRequested > supportedBindingType)
      HICR_THROW_LOGIC(
        "Requesting an allocation binding support level (%u) not supported by the operating system (HWLoc max support: %u)", _hwlocBindingRequested, supportedBindingType);

    // Getting memory space's HWLoc object to perform the operation with
    const auto hwlocObj = m->getHWLocObject();

    // Allocating memory in the reqested memory space
    void *ptr = nullptr;
    if (bindingTypeToUse == L0::LocalMemorySlot::binding_type::strict_binding)
      ptr = hwloc_alloc_membind(*_topology, size, hwlocObj->nodeset, HWLOC_MEMBIND_DEFAULT, HWLOC_MEMBIND_BYNODESET | HWLOC_MEMBIND_STRICT);
    if (bindingTypeToUse == L0::LocalMemorySlot::binding_type::strict_non_binding) ptr = malloc(size);

    // Error checking
    if (ptr == nullptr) HICR_THROW_RUNTIME("Could not allocate memory (size %lu) in the requested memory space", size);

    // Creating new memory slot object
    auto memorySlot = std::make_shared<L0::LocalMemorySlot>(supportedBindingType, ptr, size, memorySpace);

    // Assinging new entry in the memory slot map
    return memorySlot;
  }

  __INLINE__ std::shared_ptr<HiCR::L0::LocalMemorySlot> registerLocalMemorySlotImpl(std::shared_ptr<HiCR::L0::MemorySpace> memorySpace, void *const ptr, const size_t size) override
  {
    // Creating new memory slot object
    auto memorySlot = std::make_shared<L0::LocalMemorySlot>(L0::LocalMemorySlot::binding_type::strict_non_binding, ptr, size, memorySpace);

    // Returning new memory slot pointer
    return memorySlot;
  }

  __INLINE__ void deregisterLocalMemorySlotImpl(std::shared_ptr<HiCR::L0::LocalMemorySlot> memorySlot) override
  {
    // Nothing to do here
  }

  __INLINE__ void freeLocalMemorySlotImpl(std::shared_ptr<HiCR::L0::LocalMemorySlot> memorySlot) override
  {
    // Getting up-casted pointer for the execution unit
    auto m = dynamic_pointer_cast<L0::LocalMemorySlot>(memorySlot);

    // Checking whether the execution unit passed is compatible with this backend
    if (m == nullptr) HICR_THROW_LOGIC("The passed memory slot is not supported by this backend\n");

    // Getting memory slot info
    const auto memorySlotBindingType = m->getBindingType();
    const auto memorySlotPointer     = m->getPointer();
    const auto memorySlotSize        = m->getSize();

    // If using strict binding, use hwloc_free to properly unmap the memory binding
    if (memorySlotBindingType == L0::LocalMemorySlot::binding_type::strict_binding)
    {
      // Freeing memory slot
      auto status = hwloc_free(*_topology, memorySlotPointer, memorySlotSize);

      // Error checking
      if (status != 0) HICR_THROW_RUNTIME("Could not free bound memory slot.");
    }

    // If using strict non binding, use system's free
    if (memorySlotBindingType == L0::LocalMemorySlot::binding_type::strict_non_binding) { free(memorySlotPointer); }
  }
};

} // namespace HiCR::backend::hwloc::L1
