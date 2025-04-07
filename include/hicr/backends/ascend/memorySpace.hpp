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
 * @brief This file implements the memory space class for the Ascend backend
 * @author L. Terracciano & S. M. Martin
 * @date 15/12/2023
 */

#pragma once

#include <nlohmann_json/json.hpp>
#include <hicr/core/definitions.hpp>
#include <hicr/core/memorySpace.hpp>
#include <hicr/backends/ascend/localMemorySlot.hpp>

namespace HiCR::backend::ascend
{

/**
 * Forward declaration of the Ascend device class -- a not-so-elegant solution to a circular dependency, but all we can do for now
 */
class Device;

/**
 * This class represents a memory space, as visible by the Ascend backend. That is, the entire RAM that the running CPU has access to.
 */
class MemorySpace final : public HiCR::MemorySpace
{
  public:

  /**
   * Constructor for the compute resource class of the Ascend backend
   *
   * \param device The Ascend device in which this memory space was detected
   * \param size The size of this memory space
   */
  MemorySpace(const std::weak_ptr<ascend::Device> device, const size_t size)
    : HiCR::MemorySpace(size),
      _device(device){};

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

  __INLINE__ std::string getType() const override { return "Ascend Device RAM"; }

  /**
   * Function to get the Ascend device associated to this memory space
   *
   * @return The Ascend device corresponding to this memory space
   */
  __INLINE__ const std::weak_ptr<const ascend::Device> getDevice() const { return _device; }

  private:

  __INLINE__ void serializeImpl(nlohmann::json &output) const override
  {
    // No additional information to serialize for now
  }

  __INLINE__ void deserializeImpl(const nlohmann::json &input) override
  {
    // No additional information to deserialize for now
  }

  /**
   * Stores the device that owns this memory space
   *
   * \note If this class has been created through deserialization, it is not meant to be used as this pointer remains undefined
   */
  std::weak_ptr<ascend::Device> _device;
};

} // namespace HiCR::backend::ascend
