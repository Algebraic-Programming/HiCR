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
#include <hicr/L0/device.hpp>

namespace HiCR
{

namespace L0
{

/**
 * This class represents an abstract definition for a HiCR Topology that:
 *
 * - Describes the physical resources (devices) of a given system (real or constructed for requesting new instances)
 * - Information about the connectivity between the given devices
 */
class Topology
{
  public:

  /**
   * Common type for a collection of devices
   */
  typedef std::unordered_set<std::shared_ptr<L0::Device>> deviceList_t;

  Topology() = default;
  ~Topology() = default;

  /**
   * This function prompts the backend to perform the necessary steps to return  existing devices
   * \return A set of pointers to HiCR instances that refer to both local and remote instances
   */
  __USED__ inline const deviceList_t &getDevices() const { return _deviceList; }

  /**
   * This function allows manually adding a new device into an existing topology
   *
   * @param[in] device The device to add
   */
  __USED__ inline void addDevice(const std::shared_ptr<HiCR::L0::Device> device) { _deviceList.insert(device); }

  /**
   * This function allows manually merging one topology information into another
   *
   * @param[in] source The source topology to add into this instance
   */
  __USED__ inline void merge(const Topology &source)
  {
    // Adding each device separately
    for (const auto &d : source.getDevices()) addDevice(d);
  }

  /**
   * Serialization function to enable sharing topology information across different HiCR instances (or any other purposes)
   *
   * @return JSON-formatted serialized topology, as detected by this topology manager
   */
  __USED__ inline nlohmann::json serialize() const
  {
    // Storage for newly created serialized output
    nlohmann::json output;

    // Adding serialized devices information into the array
    std::string devicesKey = "Devices";
    output[devicesKey] = std::vector<nlohmann::json>();
    for (const auto &device : _deviceList) output[devicesKey] += device->serialize();

    // Returning topology
    return output;
  }

  /**
   * Verifies the provided input (encoded in JSON) satisfied the standard format to describe a topology
   *
   * @param[in] input JSON-formatted serialized topology
   */
  __USED__ static inline void verify(const nlohmann::json &input)
  {
    // Sanity checks
    if (input.contains("Devices") == false) HICR_THROW_LOGIC("Serialized topology manager information is invalid, as it lacks the 'Devices' entry");
    if (input["Devices"].is_array() == false) HICR_THROW_LOGIC("Serialized topology manager 'Devices' entry is not an array.");

    for (auto device : input["Devices"])
    {
      if (device.contains("Type") == false) HICR_THROW_LOGIC("Serialized device information is invalid, as it lacks the 'Type' entry");
      if (device["Type"].is_string() == false) HICR_THROW_LOGIC("Serialized device information is invalid, as the 'Type' entry is not a string");
    }
  };

  protected:

  /**
   * Map of devices queried by this topology manager
   */
  deviceList_t _deviceList;
};

} // namespace L0

} // namespace HiCR
