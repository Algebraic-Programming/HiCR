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
 * @brief This file implements the Device class for the acl backend
 * @author L. Terracciano & S. M. Martin
 * @date 20/12/2023
 */

#pragma once

#include <acl/acl.h>
#include <hicr/core/definitions.hpp>
#include <hicr/core/device.hpp>
#include <hicr/backends/acl/computeResource.hpp>
#include <hicr/backends/acl/memorySpace.hpp>

namespace HiCR::backend::acl
{

/**
 * This class represents a device, as visible by the acl backend.
 */
class Device final : public HiCR::Device
{
  public:

  /**
   * Type definition for the Huawei Device identifier
   */
  typedef uint64_t deviceIdentifier_t;

  /**
   * Constructor for an Huawei device
   *
   * \param id Internal unique identifier for the device
   * \param computeResources The compute resources associated to this device (typically just one, the main Huawei processor)
   * \param memorySpaces The memory spaces associated to this device (DRAM + other use-specific or high-bandwidth memories)
   */
  Device(const deviceIdentifier_t id, const computeResourceList_t &computeResources, const memorySpaceList_t &memorySpaces)
    : HiCR::Device(computeResources, memorySpaces),
      _id(id),
      _context(std::make_unique<aclrtContext>())
  {
    // create ACL context for executing operations on the given device
    aclError err = aclrtCreateContext(_context.get(), id);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not create context for device %ld. Error %d", _id, err);
  };

  /**
   * Default constructor for resource requesting
   */
  Device()
    : HiCR::Device()
  {
    _type = "Huawei Device";
  }

  /**
   * Deserializing constructor
   *
   * The instance created will contain all information, if successful in deserializing it, corresponding to the passed Huawei device
   * This instance should NOT be used for anything else than reporting/printing the contained resources
   *
   * @param[in] input A JSON-encoded serialized Huawei device information
   */
  Device(const nlohmann::json &input)
    : HiCR::Device()
  {
    deserialize(input);
  }

  /**
   * Set this device as the one on which the operations needs to be executed
   */
  __INLINE__ void select() const { selectDevice(_context.get(), _id); }

  /**
   * Device destructor
   */
  ~Device()
  {
    // destroy the ACL context
    aclError err = aclrtDestroyContext(*_context.get());
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not destroy context for device %ld. Error %d", _id, err);
  };

  /**
   * Returns the internal id of the current Huawei device
   *
   * \return The id of the Huawei device
   */
  __INLINE__ deviceIdentifier_t getId() const { return _id; }

  /**
   * Returns the ACL context corresponding to this compute resource
   *
   * \return The ACL context
   */
  __INLINE__ aclrtContext *getContext() const { return _context.get(); }

  private:

  __INLINE__ void serializeImpl(nlohmann::json &output) const override
  {
    // Storing device identifier
    output["Device Identifier"] = _id;
  }

  __INLINE__ void deserializeImpl(const nlohmann::json &input) override
  {
    // Getting device id
    std::string key = "Device Identifier";
    if (input.contains(key) == false) HICR_THROW_LOGIC("The serialized object contains no '%s' key", key.c_str());
    if (input[key].is_number() == false) HICR_THROW_LOGIC("The '%s' entry is not a number", key.c_str());
    _id = input[key].get<deviceIdentifier_t>();

    // Iterating over the compute resource list
    for (const auto &computeResource : input[_HICR_DEVICE_COMPUTE_RESOURCES_KEY_])
    {
      // Getting device type
      const auto type = computeResource["Type"].get<std::string>();

      // Checking whether the type is correct
      if (type != "Huawei Processor") HICR_THROW_LOGIC("The passed device type '%s' is not compatible with this topology manager", type.c_str());

      // Deserializing new device
      auto computeResourceObj = std::make_shared<acl::ComputeResource>(computeResource);

      // Inserting device into the list
      addComputeResource(computeResourceObj);
    }

    // Iterating over the memory space list
    for (const auto &memorySpace : input[_HICR_DEVICE_MEMORY_SPACES_KEY_])
    {
      // Getting device type
      const auto type = memorySpace["Type"].get<std::string>();

      // Checking whether the type is correct
      if (type != "Huawei Device RAM") HICR_THROW_LOGIC("The passed device type '%s' is not compatible with this topology manager", type.c_str());

      // Deserializing new device
      auto memorySpaceObj = std::make_shared<acl::MemorySpace>(memorySpace);

      // Inserting device into the list
      addMemorySpace(memorySpaceObj);
    }
  }

  /**
   * Individual identifier for the Huawei device
   */
  deviceIdentifier_t _id;

  /**
   * The internal acl context associated to the device
   */
  const std::unique_ptr<aclrtContext> _context;

  private:

  /**
   * Set the device on which the operations needs to be executed
   *
   * \param deviceContext the device ACL context
   * \param deviceId the device identifier. Needed for loggin purposes
   */
  __INLINE__ static void selectDevice(const aclrtContext *deviceContext, const deviceIdentifier_t deviceId)
  {
    // select the device context on which operations shoud be executed
    aclError err = aclrtSetCurrentContext(*deviceContext);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("can not set the device %ld context. Error %d", deviceId, err);
  }
};

} // namespace HiCR::backend::acl
