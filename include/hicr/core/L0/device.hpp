/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
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

/**
 * HiCR standard denomination of the compute resources key
 */
#define _HICR_DEVICE_COMPUTE_RESOURCES_KEY_ "Compute Resources"

/**
 * HiCR standard denomination of the memory spaces key
 */
#define _HICR_DEVICE_MEMORY_SPACES_KEY_ "Memory Spaces"

namespace HiCR
{

namespace L0
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
  typedef std::vector<std::shared_ptr<L0::ComputeResource>> computeResourceList_t;

  /**
   * Common definition of a collection of memory spaces
   */
  typedef std::vector<std::shared_ptr<L0::MemorySpace>> memorySpaceList_t;

  /**
   * Indicates what type of device is represented in this instance
   *
   * \return A string containing a human-readable description of the compute resource type
   */
  virtual std::string getType() const = 0;

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
  __INLINE__ void addComputeResource(std::shared_ptr<HiCR::L0::ComputeResource> computeResource) { _computeResources.push_back(computeResource); }

  /**
   * This function allows the deferred addition (post construction) of memory spaces
   *
   * \param[in] memorySpace The compute resource to add
   */
  __INLINE__ void addMemorySpace(std::shared_ptr<HiCR::L0::MemorySpace> memorySpace) { _memorySpaces.push_back(memorySpace); }

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
  Device(const computeResourceList_t &computeResources, const memorySpaceList_t &memorySpaces)
    : _computeResources(computeResources),
      _memorySpaces(memorySpaces){};

  /**
   * Serialization function to enable sharing device information
   *
   * @return JSON-formatted serialized device information
   */
  __INLINE__ nlohmann::json serialize() const
  {
    // Storage for newly created serialized output
    nlohmann::json output;

    // Getting device type
    output["Type"] = getType();

    // Getting device-specific serialization information
    serializeImpl(output);

    // Serializing compute resource information
    std::string computeResourceKey = _HICR_DEVICE_COMPUTE_RESOURCES_KEY_;
    output[computeResourceKey]     = std::vector<nlohmann::json>();
    for (const auto &computeResource : _computeResources) output[computeResourceKey] += computeResource->serialize();

    // Serializing memory space information
    std::string memorySpaceKey = _HICR_DEVICE_MEMORY_SPACES_KEY_;
    output[memorySpaceKey]     = std::vector<nlohmann::json>();
    for (const auto &memorySpace : _memorySpaces) output[memorySpaceKey] += memorySpace->serialize();

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
    std::string computeResourceKey = _HICR_DEVICE_COMPUTE_RESOURCES_KEY_;
    if (input.contains(computeResourceKey) == false) HICR_THROW_LOGIC("Serialized device information is invalid, as it lacks the '%s' entry", computeResourceKey.c_str());
    if (input[computeResourceKey].is_array() == false) HICR_THROW_LOGIC("Serialized device information is invalid, as '%s' entry is not an array.", computeResourceKey.c_str());
    for (const auto &c : input[computeResourceKey])
    {
      if (c.contains("Type") == false) HICR_THROW_LOGIC("In '%s', entry information is invalid, as it lacks the 'Type' entry", computeResourceKey.c_str());
      if (c["Type"].is_string() == false) HICR_THROW_LOGIC("In '%s', entry information information is invalid, as the 'Type' entry is not a string", computeResourceKey.c_str());
    }

    std::string memorySpaceKey = _HICR_DEVICE_MEMORY_SPACES_KEY_;
    if (input.contains(memorySpaceKey) == false) HICR_THROW_LOGIC("Serialized device information is invalid, as it lacks the '%s' entry", memorySpaceKey.c_str());
    if (input[memorySpaceKey].is_array() == false) HICR_THROW_LOGIC("Serialized device information is invalid, as '%s' entry is not an array.", memorySpaceKey.c_str());
    for (const auto &c : input[memorySpaceKey])
    {
      if (c.contains("Type") == false) HICR_THROW_LOGIC("In '%s', entry information is invalid, as it lacks the 'Type' entry", memorySpaceKey.c_str());
      if (c["Type"].is_string() == false) HICR_THROW_LOGIC("In '%s', entry information information is invalid, as the 'Type' entry is not a string", memorySpaceKey.c_str());
    }

    // Then call the backend-specific deserialization function
    deserializeImpl(input);

    // Checking whether the deserialization was successful
    if (_computeResources.size() != input[computeResourceKey].size())
      HICR_THROW_LOGIC("Deserialization failed, as the number of compute resources created (%lu) differs from the ones provided in the serialized input (%lu)",
                       _memorySpaces.size(),
                       input[computeResourceKey].size());
    if (_memorySpaces.size() != input[memorySpaceKey].size())
      HICR_THROW_LOGIC("Deserialization failed, as the number of memory spaces created (%lu) differs from the ones provided in the serialized input (%lu)",
                       _memorySpaces.size(),
                       input[memorySpaceKey].size());
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

} // namespace L0

} // namespace HiCR
