/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file memorySpace.hpp
 * @brief This file implements the memory space class for the shared memory backend
 * @author S. M. Martin
 * @date 12/12/2023
 */

#pragma once

#include "hwloc.h"
#include <backends/sharedMemory/L0/localMemorySlot.hpp>
#include <hicr/L0/memorySpace.hpp>
#include <hicr/common/definitions.hpp>

namespace HiCR
{

namespace backend
{

namespace sharedMemory
{

namespace L0
{

/**
 * This class represents a memory space, as visible by the sequential backend. That is, the entire RAM that the running CPU has access to.
 */
class MemorySpace final : public HiCR::L0::MemorySpace
{
  public:

  /**
   * Constructor for the memory space class of the shared memory backend
   *
   * \param size The maximum allocatable size detected for this memory space
   * \param hwlocObject HWLoc object for associated to this memory space
   * \param bindingSupport The HWLoc binding type supported by this memory space
   */
  MemorySpace(const size_t size, const hwloc_obj_t hwlocObject, const sharedMemory::L0::LocalMemorySlot::binding_type bindingSupport) : HiCR::L0::MemorySpace(size),
                                                                                                                                        _hwlocObject(hwlocObject),
                                                                                                                                        _bindingSupport(bindingSupport){};

  /**
   * Default destructor
   */
  ~MemorySpace() = default;

  __USED__ inline std::string getType() const override { return "NUMA Domain"; }

  /**
   * Function to determine whether the memory space supports strictly bound memory allocations
   *
   * @return The supported memory binding type by the memory space
   */
  __USED__ inline sharedMemory::L0::LocalMemorySlot::binding_type getSupportedBindingType() const
  {
    return _bindingSupport;
  }

  /**
   * Function to get the internal HWLoc object represented by this memory space
   *
   * @return The internal HWLoc object
   */
  __USED__ inline const hwloc_obj_t getHWLocObject() const
  {
    return _hwlocObject;
  }

  private:

  /**
   * HWloc object representing this memory space
   */
  const hwloc_obj_t _hwlocObject;

  /**
   * Stores whether it is possible to allocate bound memory in this memory space
   */
  const sharedMemory::L0::LocalMemorySlot::binding_type _bindingSupport;
};

} // namespace L0

} // namespace sharedMemory

} // namespace backend

} // namespace HiCR
