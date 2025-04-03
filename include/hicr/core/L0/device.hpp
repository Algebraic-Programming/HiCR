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
 * @brief Provides a base definition for a HiCR Device class
 * @author S. M. Martin
 * @date 18/12/2023
 */
#pragma once

#include <memory>
#include <unordered_set>
#include <nlohmann_json/json.hpp>
#include <hicr/core/L0/computeResource.hpp>
#include <hicr/core/L0/memorySpace.hpp>
#include <utility>

/**
 * HiCR standard denomination of the compute resources key
 */
constexpr std::string_view _HICR_DEVICE_COMPUTE_RESOURCES_KEY_ = "Compute Resources";

/**
 * HiCR standard denomination of the memory spaces key
 */
constexpr std::string_view _HICR_DEVICE_MEMORY_SPACES_KEY_ = "Memory Spaces";

namespace HiCR::L0
{

/**
 * This class represents an abstract definition for a HiCR Device that:
 *
 * - Represents a physical computing device (e.g., CPU+RAM, GPU+DRAM), containing a set of compute resources (e.g., cores) and/or memory spaces (e.g., RAM)
 * - It may contain information about the connectivity between its compute and memory resources
 * - This is a copy-able class that only contains metadata
 */
class Device
{
  public:

  /**
   * Common type for a collection of compute resources
   */
  using computeResourceList_t = std::vector<std::shared_ptr<L0::ComputeResource>>;

  /**
   * Common definition of a collection of memory spaces
   */
  using memorySpaceList_t = std::vector<std::shared_ptr<L0::MemorySpace>>;

  /**
   * Indicates what type of device is represented in this instance
   *
   * \return A string containing a human-readable description of the compute resource type
   */
  [[nodiscard]] virtual std::string getType() const = 0;

  /**
   * This function returns the list of queried compute resources, as visible by the device.
   *
   * \return The list of compute resources contained in this device
   */
  __INLINE__ const computeResourceList_t &getComputeResourceList() { return _computeResources; }

  /**
   * This function returns the list of queried memory spaces, as visible by the device.
   *
   * \return The list of memory spaces contained in this device
   */
  __INLINE__ const memorySpaceList_t &getMemorySpaceList() { return _memorySpaces; }

  /**
   * This function allows the deferred addition (post construction) of compute resources
   *
   * \param[in] computeResource The compute resource to add
   */
  __INLINE__ void addComputeResource(const std::shared_ptr<HiCR::L0::ComputeResource> &computeResource) { _computeResources.push_back(computeResource); }

  /**
   * This function allows the deferred addition (post construction) of memory spaces
   *
   * \param[in] memorySpace The compute resource to add
   */
  __INLINE__ void addMemorySpace(const std::shared_ptr<HiCR::L0::MemorySpace> &memorySpace) { _memorySpaces.push_back(memorySpace); }

  /**
   * Default destructor
   */
  virtual ~Device() = default;

  /**
   *  Constructor requires at least to provide the initial set of compute resources and memory spaces
   *
   * \param[in] computeResources The list of detected compute resources contained in this device
   * \param[in] memorySpaces The list of detected memory spaces contained in this device
   */
  Device(computeResourceList_t computeResources, memorySpaceList_t memorySpaces)
    : _computeResources(std::move(computeResources)),
      _memorySpaces(std::move(memorySpaces)){};

  /**
   * Serialization function to enable sharing device information
   *
   * @return JSON-formatted serialized device information
   */
  [[nodiscard]] __INLINE__ nlohmann::json serialize() const
  {
    // Storage for newly created serialized output
    nlohmann::json output;

    // Getting device type
    output["Type"] = getType();

    // Getting device-specific serialization information
    serializeImpl(output);

    // Serializing compute resource information
    output[_HICR_DEVICE_COMPUTE_RESOURCES_KEY_] = std::vector<nlohmann::json>();
    for (const auto &computeResource : _computeResources) output[_HICR_DEVICE_COMPUTE_RESOURCES_KEY_] += computeResource->serialize();

    // Serializing memory space information
    output[_HICR_DEVICE_MEMORY_SPACES_KEY_] = std::vector<nlohmann::json>();
    for (const auto &memorySpace : _memorySpaces) output[_HICR_DEVICE_MEMORY_SPACES_KEY_] += memorySpace->serialize();

    // Returning serialized information
    return output;
  }

  /**
   * De-serialization function to re-construct the serialized device information coming (typically) from remote instances
   *
   * @param[in] input JSON-formatted serialized device information
   *
   * \note Important: Deserialized devices are not meant to be used in any from other than printing or reporting its topology.
   *       Any attempt of actually using them for computation or data transfers will result in undefined behavior.
   */
  __INLINE__ void deserialize(const nlohmann::json &input)
  {
    // First, discard all existing information
    _computeResources.clear();
    _memorySpaces.clear();

    // Sanity checks
    if (input.contains(_HICR_DEVICE_COMPUTE_RESOURCES_KEY_) == false)
      HICR_THROW_LOGIC("Serialized device information is invalid, as it lacks the '%s' entry", _HICR_DEVICE_COMPUTE_RESOURCES_KEY_);
    if (input[_HICR_DEVICE_COMPUTE_RESOURCES_KEY_].is_array() == false)
      HICR_THROW_LOGIC("Serialized device information is invalid, as '%s' entry is not an array.", _HICR_DEVICE_COMPUTE_RESOURCES_KEY_);
    for (const auto &c : input[_HICR_DEVICE_COMPUTE_RESOURCES_KEY_])
    {
      if (c.contains("Type") == false) HICR_THROW_LOGIC("In '%s', entry information is invalid, as it lacks the 'Type' entry", _HICR_DEVICE_COMPUTE_RESOURCES_KEY_);
      if (c["Type"].is_string() == false)
        HICR_THROW_LOGIC("In '%s', entry information information is invalid, as the 'Type' entry is not a string", _HICR_DEVICE_COMPUTE_RESOURCES_KEY_);
    }

    if (input.contains(_HICR_DEVICE_MEMORY_SPACES_KEY_) == false)
      HICR_THROW_LOGIC("Serialized device information is invalid, as it lacks the '%s' entry", _HICR_DEVICE_MEMORY_SPACES_KEY_);
    if (input[_HICR_DEVICE_MEMORY_SPACES_KEY_].is_array() == false)
      HICR_THROW_LOGIC("Serialized device information is invalid, as '%s' entry is not an array.", _HICR_DEVICE_MEMORY_SPACES_KEY_);
    for (const auto &c : input[_HICR_DEVICE_MEMORY_SPACES_KEY_])
    {
      if (c.contains("Type") == false) HICR_THROW_LOGIC("In '%s', entry information is invalid, as it lacks the 'Type' entry", _HICR_DEVICE_MEMORY_SPACES_KEY_);
      if (c["Type"].is_string() == false)
        HICR_THROW_LOGIC("In '%s', entry information information is invalid, as the 'Type' entry is not a string", _HICR_DEVICE_MEMORY_SPACES_KEY_);
    }

    // Then call the backend-specific deserialization function
    deserializeImpl(input);

    // Checking whether the deserialization was successful
    if (_computeResources.size() != input[_HICR_DEVICE_COMPUTE_RESOURCES_KEY_].size())
      HICR_THROW_LOGIC("Deserialization failed, as the number of compute resources created (%lu) differs from the ones provided in the serialized input (%lu)",
                       _memorySpaces.size(),
                       input[_HICR_DEVICE_COMPUTE_RESOURCES_KEY_].size());
    if (_memorySpaces.size() != input[_HICR_DEVICE_MEMORY_SPACES_KEY_].size())
      HICR_THROW_LOGIC("Deserialization failed, as the number of memory spaces created (%lu) differs from the ones provided in the serialized input (%lu)",
                       _memorySpaces.size(),
                       input[_HICR_DEVICE_MEMORY_SPACES_KEY_].size());
  };

  protected:

  /**
   * Protected constructor only for the derived classes to run the deserializing constructor
   */
  Device() = default;

  /**
   * Backend-specific implementation of the serialize function
   *
   * @param[out] output Serialized device information
   */
  virtual void serializeImpl(nlohmann::json &output) const = 0;

  /**
   * Backend-specific implementation of the deserialize function
   *
   * @param[in] input Serialized device information
   */
  virtual void deserializeImpl(const nlohmann::json &input) = 0;

  private:

  /**
   * Set of compute resources contained in this device.
   */
  computeResourceList_t _computeResources;

  /**
   * Set of memory spaces contained in this device.
   */
  memorySpaceList_t _memorySpaces;
};

} // namespace HiCR::L0
