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
 * @brief This file implements the Device class for the OpenCL backend
 * @author L. Terracciano
 * @date 06/03/2025
 */

#pragma once

#include <hicr/core/definitions.hpp>
#include <hicr/core/device.hpp>
#include <hicr/backends/opencl/computeResource.hpp>
#include <hicr/backends/opencl/memorySpace.hpp>

namespace HiCR::backend::opencl
{
/**
 * This class represents a device, as visible by the OpenCL backend.
 */
class Device final : public HiCR::Device
{
  public:

  /**
   * Type definition for the OpenCL Device identifier
   */
  typedef uint64_t deviceIdentifier_t;

  /**
   * Constructor for an OpenCL device
   *
   * \param id Internal unique identifier for the device
   * \param type Device type
   * \param device OpenCL device class
   * \param computeResources The compute resources associated to this device (typically just one, the main OpenCL processor)
   * \param memorySpaces The memory spaces associated to this device (DRAM + other use-specific or high-bandwidth memories)
   */
  Device(const deviceIdentifier_t           id,
         const std::string                 &type,
         const std::shared_ptr<cl::Device> &device,
         const computeResourceList_t       &computeResources,
         const memorySpaceList_t           &memorySpaces)
    : HiCR::Device(computeResources, memorySpaces),
      _id(id),
      _type(type),
      _device(device) {};

  /**
   * Default constructor for resource requesting
   */
  Device()
    : HiCR::Device()
  {
    _type = "OpenCL Device";
  }

  /**
   * Deserializing constructor
   *
   * The instance created will contain all information, if successful in deserializing it, corresponding to the passed OpenCL device
   * This instance should NOT be used for anything else than reporting/printing the contained resources
   *
   * @param[in] input A JSON-encoded serialized OpenCL device information
   */
  Device(const nlohmann::json &input)
    : HiCR::Device()
  {
    deserialize(input);
  }

  /**
   * Device destructor
   */
  ~Device() {

  };

  /**
   * Get device id
   * 
   * \return device id
  */
  __INLINE__ deviceIdentifier_t getId() const { return _id; }

  /**
   * Get OpenCL Device 
   * 
   * \return OpenCL Device
  */
  __INLINE__ const cl::Device &getOpenCLDevice() const { return *_device; }

  private:

  __INLINE__ void serializeImpl(nlohmann::json &output) const override
  {
    output["Device Identifier"] = _id;
    output["Device Type"]       = _type;
  }

  __INLINE__ void deserializeImpl(const nlohmann::json &input) override
  {
    std::string idKey   = "Device Identifier";
    std::string typeKey = "Device Type";

    if (input.contains(idKey) == false) HICR_THROW_LOGIC("The serialized object contains no '%s' key", idKey.c_str());
    if (input[idKey].is_number() == false) HICR_THROW_LOGIC("The '%s' entry is not a number", idKey.c_str());

    if (input.contains(typeKey) == false) HICR_THROW_LOGIC("The serialized object contains no '%s' key", typeKey.c_str());
    if (input[typeKey].is_number() == false) HICR_THROW_LOGIC("The '%s' entry is not a number", typeKey.c_str());

    _id   = input[idKey].get<deviceIdentifier_t>();
    _type = input[typeKey].get<std::string>();

    for (const auto &computeResource : input[_HICR_DEVICE_COMPUTE_RESOURCES_KEY_])
    {
      // Getting device type
      const auto type = computeResource["Type"].get<std::string>();

      // Checking whether the type is correct
      if (type != getType() + " Processing Unit") HICR_THROW_LOGIC("The passed device type '%s' is not compatible with this topology manager", type.c_str());

      // Deserializing new device
      auto computeResourceObj = std::make_shared<opencl::ComputeResource>(computeResource);

      // Inserting device into the list
      this->addComputeResource(computeResourceObj);
    }

    // Iterating over the memory space list
    for (const auto &memorySpace : input[_HICR_DEVICE_MEMORY_SPACES_KEY_])
    {
      // Getting device type
      const auto type = memorySpace["Type"].get<std::string>();

      // Checking whether the type is correct
      if (type != getType() + " RAM") HICR_THROW_LOGIC("The passed device type '%s' is not compatible with this topology manager", type.c_str());

      // Deserializing new device
      auto memorySpaceObj = std::make_shared<opencl::MemorySpace>(memorySpace);

      // Inserting device into the list
      this->addMemorySpace(memorySpaceObj);
    }
  }

  /**
   * Individual identifier for the OpenCL device
   */
  deviceIdentifier_t _id;

  /**
   * String reprensenting the device type
  */
  std::string _type;

  /**
   * OpenCL device
  */
  const std::shared_ptr<cl::Device> _device;
};

} // namespace HiCR::backend::opencl
