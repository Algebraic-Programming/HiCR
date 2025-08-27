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
 * @brief This file implements the memory space class for the OpenCL backend
 * @author L. Terracciano
 * @date 06/03/2025
 */

#pragma once

#include <nlohmann_json/json.hpp>
#include <hicr/core/definitions.hpp>
#include <hicr/core/memorySpace.hpp>
#include <hicr/backends/opencl/localMemorySlot.hpp>

namespace HiCR::backend::opencl
{

/**
 * Forward declaration of the OpenCL device class -- a not-so-elegant solution to a circular dependency, but all we can do for now
 */
class Device;

/**
 * This class represents a memory space, as visible by the OpenCL backend. That is, the entire accessible RAM
 */
class MemorySpace final : public HiCR::MemorySpace
{
  public:

  /**
   * Constructor for the compute resource class of the OpenCL backend
   *
   * \param device The OpenCL device in which this memory space was detected
   * \param size The size of this memory space
   */
  MemorySpace(const std::weak_ptr<opencl::Device> device, const size_t size)
    : HiCR::MemorySpace(size),
      _device(device)
  {
    _type = "OpenCL Device RAM";
  };

  /**
   * Default constructor for resource requesting
   */
  MemorySpace()
    : HiCR::MemorySpace()
  {
    _type = "OpenCL Device RAM";
  }

  /**
   * Default destructor
   */
  ~MemorySpace() = default;

  /**
   * Deserializing constructor
   *
   * @param[in] input Serialized resource information
   *
   * \note Backwards reference to device is null when deserializing. Do not try to use this class for any operations.
   */
  MemorySpace(const nlohmann::json &input)
    : HiCR::MemorySpace()
  {
    deserialize(input);
  }

  /**
   * Function to get the OpenCL device associated to this memory space
   *
   * @return The OpenCL device corresponding to this memory space
   */
  __INLINE__ const std::weak_ptr<const opencl::Device> getDevice() const { return _device; }

  private:

  __INLINE__ void serializeImpl(nlohmann::json &output) const override { output["Memory Space Type"] = _type; }

  __INLINE__ void deserializeImpl(const nlohmann::json &input) override
  {
    std::string typeKey = "Memory Space Type";
    if (input.contains(typeKey) == false) HICR_THROW_LOGIC("The serialized object contains no '%s' key", typeKey.c_str());
    if (input[typeKey].is_number() == false) HICR_THROW_LOGIC("The '%s' entry is not a number", typeKey.c_str());
    _type = input[typeKey].get<std::string>();
  }

  /**
   * Stores the device that owns this memory space
   *
   * \note If this class has been created through deserialization, it is not meant to be used as this pointer remains undefined
   */
  std::weak_ptr<opencl::Device> _device;

  /**
   * Memory space device type
  */
  std::string _type;
};

} // namespace HiCR::backend::opencl
