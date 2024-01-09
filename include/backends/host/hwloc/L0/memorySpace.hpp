/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file memorySpace.hpp
 * @brief This file implements the memory space class for the HWLoc-based backend
 * @author S. M. Martin
 * @date 12/12/2023
 */

#pragma once

#include "hwloc.h"
#include <hicr/definitions.hpp>
#include <backends/host/L0/memorySpace.hpp>
#include <backends/host/hwloc/L0/localMemorySlot.hpp>

namespace HiCR
{

namespace backend
{

namespace host
{

namespace hwloc
{

namespace L0
{

/**
 * This class represents a memory space, as visible by the sequential backend. That is, the entire RAM that the running CPU has access to.
 */
class MemorySpace final : public HiCR::backend::host::L0::MemorySpace
{
  public:

  /**
   * Constructor for the memory space class of the shared memory backend
   *
   * \param size The maximum allocatable size detected for this memory space
   * \param hwlocObject HWLoc object for associated to this memory space
   * \param bindingSupport The HWLoc binding type supported by this memory space
   */
  MemorySpace(const size_t size, const hwloc_obj_t hwlocObject, const host::hwloc::L0::LocalMemorySlot::binding_type bindingSupport) : HiCR::backend::host::L0::MemorySpace(size),
                                                                                                                                       _hwlocObject(hwlocObject),
                                                                                                                                       _bindingSupport(bindingSupport){};

  /**
   * Deserializing constructor
   *
   * The instance created will contain all information, if successful in deserializing it, corresponding to the passed host RAM
   * This instance should NOT be used for anything else than reporting/printing the contained resources
   *
   * @param[in] input A JSON-encoded serialized host RAM information
   */
  MemorySpace(const nlohmann::json &input) : HiCR::backend::host::L0::MemorySpace()
  {
    deserialize(input);
  }

  /**
   * Default destructor
   */
  ~MemorySpace() = default;

  /**
   * Function to determine whether the memory space supports strictly bound memory allocations
   *
   * @return The supported memory binding type by the memory space
   */
  __USED__ inline host::hwloc::L0::LocalMemorySlot::binding_type getSupportedBindingType() const
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

  __USED__ inline void serializeImpl(nlohmann::json &output) const override
  {
    // Writing HWLoc-specific information into the serialized object
    output["Binding Support"] = _bindingSupport;
  }

  __USED__ inline void deserializeImpl(const nlohmann::json &input) override
  {
    std::string key = "Binding Support";
    if (input.contains(key) == false) HICR_THROW_LOGIC("The serialized object contains no '%s' key", key.c_str());
    if (input[key].is_number() == false) HICR_THROW_LOGIC("The '%s' entry is not a number", key.c_str());
    _bindingSupport = input[key].get<host::hwloc::L0::LocalMemorySlot::binding_type>();
  }

  /**
   * HWloc object representing this memory space
   */
  hwloc_obj_t _hwlocObject;

  /**
   * Stores whether it is possible to allocate bound memory in this memory space
   */
  host::hwloc::L0::LocalMemorySlot::binding_type _bindingSupport;
};

} // namespace L0

} // namespace hwloc

} // namespace host

} // namespace backend

} // namespace HiCR
