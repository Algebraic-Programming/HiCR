/*
 *   Copyright 2025 Huawei Technologies Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file memorySpace.hpp
 * @brief This file implements the memory space class for the HWLoc-based backend
 * @author S. M. Martin
 * @date 12/12/2023
 */

#pragma once

#include <hwloc.h>
#include <hicr/core/definitions.hpp>
#include <hicr/core/memorySpace.hpp>
#include "localMemorySlot.hpp"

namespace HiCR::backend::hwloc
{

/**
 * This class represents a memory space, as visible by the hwloc backend. That is, the entire RAM that the running CPU has access to.
 */
class MemorySpace final : public HiCR::MemorySpace
{
  public:

  /**
   * Constructor for the memory space class of the hwloc backend
   *
   * \param size The maximum allocatable size detected for this memory space
   * \param hwlocObject HWLoc object for associated to this memory space
   * \param bindingSupport The HWLoc binding type supported by this memory space
   */
  MemorySpace(const size_t size, const hwloc_obj_t hwlocObject, const hwloc::LocalMemorySlot::binding_type bindingSupport)
    : HiCR::MemorySpace(size),
      _hwlocObject(hwlocObject),
      _bindingSupport(bindingSupport)
  {
    _type = "RAM";
  };

  /**
   * Default destructor
   */
  ~MemorySpace() override = default;

  /**
   * Function to determine whether the memory space supports strictly bound memory allocations
   *
   * @return The supported memory binding type by the memory space
   */
  [[nodiscard]] __INLINE__ hwloc::LocalMemorySlot::binding_type getSupportedBindingType() const { return _bindingSupport; }

  /**
   * Function to get the internal HWLoc object represented by this memory space
   *
   * @return The internal HWLoc object
   */
  [[nodiscard]] __INLINE__ const hwloc_obj_t getHWLocObject() const { return _hwlocObject; }

  private:

  __INLINE__ void serializeImpl(nlohmann::json &output) const override
  {
    // Writing HWLoc-specific information into the serialized object
    output["Binding Support"] = _bindingSupport;
  }

  __INLINE__ void deserializeImpl(const nlohmann::json &input) override
  {
    // Checking whether the type is correct
    if (_type != "RAM") HICR_THROW_LOGIC("The passed memory space type '%s' is not compatible with this topology manager", _type.c_str());

    std::string key = "Binding Support";
    if (input.contains(key) == false) HICR_THROW_LOGIC("The serialized object contains no '%s' key", key.c_str());
    if (input[key].is_number() == false) HICR_THROW_LOGIC("The '%s' entry is not a number", key.c_str());
    _bindingSupport = input[key].get<hwloc::LocalMemorySlot::binding_type>();
  }

  /**
   * HWloc object representing this memory space
   */
  hwloc_obj_t _hwlocObject{};

  /**
   * Stores whether it is possible to allocate bound memory in this memory space
   */
  hwloc::LocalMemorySlot::binding_type _bindingSupport{hwloc::LocalMemorySlot::binding_type::strict_binding};
};

} // namespace HiCR::backend::hwloc
