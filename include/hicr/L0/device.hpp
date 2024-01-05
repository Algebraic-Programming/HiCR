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
#include <hicr/L0/computeResource.hpp>
#include <hicr/L0/memorySpace.hpp>

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
  typedef std::unordered_set<std::shared_ptr<L0::ComputeResource>> computeResourceList_t;

  /**
   * Common definition of a collection of memory spaces
   */
  typedef std::unordered_set<std::shared_ptr<L0::MemorySpace>> memorySpaceList_t;

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
  __USED__ inline const computeResourceList_t &getComputeResourceList() { return _computeResources; }

  /**
   * This function returns the list of queried memory spaces, as visible by the device.
   *
   * \return The list of memory spaces contained in this device
   */
  __USED__ inline const memorySpaceList_t &getMemorySpaceList() { return _memorySpaces; }

  /**
   * This function allows the deferred addition (post construction) of compute resources
   *
   * \param[in] computeResource The compute resource to add
   */
  __USED__ inline void addComputeResource(std::shared_ptr<HiCR::L0::ComputeResource> computeResource) { _computeResources.insert(computeResource); }

  /**
   * This function allows the deferred addition (post construction) of memory spaces
   *
   * \param[in] memorySpace The compute resource to add
   */
  __USED__ inline void addMemorySpace(std::shared_ptr<HiCR::L0::MemorySpace> memorySpace) { _memorySpaces.insert(memorySpace); }

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
  __USED__ inline nlohmann::json serialize() const
  {
    // Storage for newly created serialized output
    nlohmann::json output;

    // Getting device type
    output["Type"] = getType();

    // Serializing compute resource information
    std::string computeResourceKey = "Compute Resources";
    output[computeResourceKey] = std::vector<nlohmann::json>();
    for (const auto& computeResource : _computeResources) output[computeResourceKey] += computeResource->serialize();

    // Serializing memory space information
    std::string memorySpaceKey = "Memory Spaces";
    output[memorySpaceKey] = std::vector<nlohmann::json>();
    for (const auto& memorySpace : _memorySpaces) output[memorySpaceKey] += memorySpace->serialize();

    // Returning serialized information
    return output;
  }

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
