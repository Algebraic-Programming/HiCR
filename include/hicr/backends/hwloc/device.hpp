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
 * @file device.hpp
 * @brief This file implements the Device class for the hwloc backend
 * @author S. M. Martin
 * @date 18/12/2023
 */

#pragma once

#include <hicr/core/definitions.hpp>
#include <hicr/core/device.hpp>
#include "computeResource.hpp"
#include "memorySpace.hpp"

namespace HiCR::backend::hwloc
{

/**
 * This class represents a device, as visible by the hwloc backend. That is, an assumed SMP processor plus a shared RAM that all process have access to.
 */
class Device final : public HiCR::Device
{
  public:

  /**
   * Type definition for a NUMA Domain identifier
   */
  using NUMADomainID_t = unsigned int;

  /**
   * Constructor for the device class of the HWLoC backend
   *
   * @param[in] NUMADomainId The OS-given NUMA domain identifier represented by this class
   * @param[in] computeResources The compute resources (cores or hyperthreads) detected in this device (CPU)
   * @param[in] memorySpaces The memory spaces (e.g., NUMA domains) detected in this device (CPU)
   */
  Device(const NUMADomainID_t NUMADomainId, const computeResourceList_t &computeResources, const memorySpaceList_t &memorySpaces)
    : HiCR::Device(computeResources, memorySpaces),
      _NUMADomainId(NUMADomainId){};

  /**
   * Empty constructor for serialization / deserialization
   */
  Device()
    : HiCR::Device(){};

  /**
   * Deserializing constructor
   *
   * The instance created will contain all information, if successful in deserializing it, corresponding to the passed NUMA domain
   * This instance should NOT be used for anything else than reporting/printing the contained resources
   *
   * @param[in] input A JSON-encoded serialized NUMA domain information
   */
  Device(const nlohmann::json &input)
    : HiCR::Device()
  {
    deserialize(input);
  }

  /**
   * Default destructor
   */
  ~Device() override = default;

  protected:

  [[nodiscard]] __INLINE__ std::string getType() const override { return "NUMA Domain"; }

  private:

  /**
   * Identifier for the NUMA domain represented by this class
   */
  NUMADomainID_t _NUMADomainId{};

  private:

  __INLINE__ void serializeImpl(nlohmann::json &output) const override
  {
    // Storing numa domain identifier
    output["NUMA Domain Id"] = _NUMADomainId;
  }

  __INLINE__ void deserializeImpl(const nlohmann::json &input) override
  {
    // Getting device id
    std::string key = "NUMA Domain Id";
    if (input.contains(key) == false) HICR_THROW_LOGIC("The serialized object contains no '%s' key", key.c_str());
    if (input[key].is_number() == false) HICR_THROW_LOGIC("The '%s' entry is not a number", key.c_str());
    _NUMADomainId = input[key].get<NUMADomainID_t>();

    // Iterating over the compute resource list
    for (const auto &computeResource : input[_HICR_DEVICE_COMPUTE_RESOURCES_KEY_])
    {
      // Getting device type
      const auto type = computeResource["Type"].get<std::string>();

      // Checking whether the type is correct
      if (type != "Processing Unit") HICR_THROW_LOGIC("The passed device type '%s' is not compatible with this topology manager", type.c_str());

      // Deserializing new device
      auto computeResourceObj = std::make_shared<hwloc::ComputeResource>(computeResource);

      // Inserting device into the list
      this->addComputeResource(computeResourceObj);
    }

    // Iterating over the memory space list
    for (const auto &memorySpace : input[_HICR_DEVICE_MEMORY_SPACES_KEY_])
    {
      // Getting device type
      const auto type = memorySpace["Type"].get<std::string>();

      // Checking whether the type is correct
      if (type != "RAM") HICR_THROW_LOGIC("The passed device type '%s' is not compatible with this topology manager", type.c_str());

      // Deserializing new device
      auto memorySpaceObj = std::make_shared<hwloc::MemorySpace>(memorySpace);

      // Inserting device into the list
      this->addMemorySpace(memorySpaceObj);
    }
  }
};

} // namespace HiCR::backend::hwloc
