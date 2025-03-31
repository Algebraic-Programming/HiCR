/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file device.hpp
 * @brief Provides a base definition for a HiCR Topology class
 * @author S. M. Martin
 * @date 18/12/2023
 */
#pragma once

#include <memory>
#include <unordered_set>
#include <nlohmann_json/json.hpp>
#include <hicr/core/L0/device.hpp>

namespace HiCR::L0
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
  using deviceList_t = std::vector<std::shared_ptr<L0::Device>>;

  Topology()  = default;
  ~Topology() = default;

  /**
   * This function prompts the backend to perform the necessary steps to return  existing devices
   * \return A set of pointers to HiCR instances that refer to both local and remote instances
   */
  [[nodiscard]] __INLINE__ const deviceList_t &getDevices() const { return _deviceList; }

  /**
   * This function allows manually adding a new device into an existing topology
   *
   * @param[in] device The device to add
   */
  __INLINE__ void addDevice(const std::shared_ptr<HiCR::L0::Device> &device) { _deviceList.push_back(device); }

  /**
   * This function allows manually merging one topology information into another
   *
   * @param[in] source The source topology to add into this instance
   */
  __INLINE__ void merge(const Topology &source)
  {
    // Adding each device separately
    for (const auto &d : source.getDevices()) addDevice(d);
  }

  /**
   * Serialization function to enable sharing topology information across different HiCR instances (or any other purposes)
   *
   * @return JSON-formatted serialized topology, as detected by this topology manager
   */
  [[nodiscard]] __INLINE__ nlohmann::json serialize() const
  {
    // Storage for newly created serialized output
    nlohmann::json output;

    // Adding serialized devices information into the array
    std::string devicesKey = "Devices";
    output[devicesKey]     = std::vector<nlohmann::json>();
    for (const auto &device : _deviceList) output[devicesKey] += device->serialize();

    // Adding metadata, if defined
    output["Metadata"] = _metadata;

    // Returning topology
    return output;
  }

  /**
   * Verifies the provided input (encoded in JSON) satisfied the standard format to describe a topology
   *
   * @param[in] input JSON-formatted serialized topology
   */
  __INLINE__ static void verify(const nlohmann::json &input)
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

  /**
   * A function to get internal topology metadata
   * 
   * @return The internal topology metadata
  */
  nlohmann::json &getMetadata() { return _metadata; }

  /**
   * A const variant of the function to get internal topology metadata
   * 
   * @return The internal topology metadata
  */
  [[nodiscard]] const nlohmann::json &getMetadata() const { return _metadata; }

  /**
   * A function to set internal topology metadata
   * 
   * @param[in] metadata The JSON-based metadata to store
  */
  void setMetadata(const nlohmann::json &metadata) { _metadata = metadata; }

  private:

  /**
   * Optional metadata storage for specifying anything about the topology that does not fit within the device class.
   * 
   * Note: This must be use sparingly as its misuse can break the implementation abstraction guarantee in HiCR
  */
  nlohmann::json _metadata;

  /**
   * Map of devices queried by this topology manager
   */
  deviceList_t _deviceList;
};

} // namespace HiCR::L0
