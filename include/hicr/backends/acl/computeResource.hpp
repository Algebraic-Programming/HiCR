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
 * @file computeResource.hpp
 * @brief This file implements the compute resource class for the acl backend
 * @author L. Terracciano & S. M. Martin
 * @date 15/12/2023
 */

#pragma once

#include <nlohmann_json/json.hpp>
#include <hicr/core/definitions.hpp>
#include <hicr/core/exceptions.hpp>
#include <hicr/core/computeResource.hpp>

namespace HiCR::backend::acl
{
/**
 * Forward declaration of the Huawei device class -- a not-so-elegant solution to a circular dependency, but all we can do for now
 */
class Device;

/**
 * This class represents a compute resource, visible by the acl backend. That is, a processing unit (Huawei device) with information about the acl context.
 */
class ComputeResource final : public HiCR::ComputeResource
{
  public:

  /**
   * Constructor for the compute resource class of the acl backend
   *
   * \param device The Huawei device that contains this compute resource
   */
  ComputeResource(const std::shared_ptr<acl::Device> &device)
    : HiCR::ComputeResource(),
      _device(device)
  {
    _type = "Huawei Processor";
  };

  /**
   * Default destructor
   */
  ~ComputeResource() = default;

  /**
   * Deserializing constructor
   *
   * @param[in] input Serialized resource information
   *
   * \note Backwards reference to device is null when deserializing. Do not try to use this class for any operations.
   */
  ComputeResource(const nlohmann::json &input)
    : HiCR::ComputeResource()
  {
    deserialize(input);
  }

  /**
   * Function to get the device id associated to this memory space
   *
   * @return The device id corresponding to this memory space
   */
  __INLINE__ const std::weak_ptr<const acl::Device> getDevice() const { return _device; }

  private:

  __INLINE__ void serializeImpl(nlohmann::json &output) const override
  {
    // Nothing extra to serialize here
  }

  __INLINE__ void deserializeImpl(const nlohmann::json &input) override
  {
    // Nothing extra  to deserialize here
  }

  /**
   * Stores the device that owns this compute resource
   *
   * \note If this class has been created through deserialization, it is not meant to be used as this pointer remains undefined
   */
  std::weak_ptr<acl::Device> _device;
};

} // namespace HiCR::backend::acl
