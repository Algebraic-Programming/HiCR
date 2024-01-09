/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file topologyManager.hpp
 * @brief Provides a definition for the abstract device manager class
 * @author S. M. Martin
 * @date 18/12/2023
 */

#pragma once

#include <nlohmann_json/json.hpp>
#include <memory>
#include <hicr/L0/device.hpp>
#include <hicr/L1/memoryManager.hpp>

namespace HiCR
{

namespace L1
{

/**
 * Encapsulates a HiCR Backend Device Manager.
 *
 * The purpose of this class is to discover the computing topology for a given device type.
 * E.g., if this is a backend for an NPU device and the system contains 8 such devices, it will discover an
 * array of Device type of size 8.
 *
 * Backends need to fulfill the abstract virtual functions described here, so that HiCR can detect hardware devices
 */
class TopologyManager
{
  public:

  /**
   * Common type for a collection of devices
   */
  typedef std::unordered_set<std::shared_ptr<L0::Device>> deviceList_t;

  /**
   * Default constructor is allowed, as no default argument are expected for the creation of this class
   */
  TopologyManager() = default;

  /**
   * Deserializing constructor is used to build a new instance of this topology manager based on serialized information
   *
   * \note The instance created by this constructor should only be used to print/query the topology. It cannot be used to operate (memcpy, compute, etc).
   *
   * @param[in] input JSON-formatted serialized topology, as detected by a remote topology manager
   */
  TopologyManager(const nlohmann::json &input) { deserialize(input); };

  /**
   * Default destructor
   */
  virtual ~TopologyManager() = default;

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
   * De-serialization function to re-construct the serialized topology information coming (typically) from remote instances
   *
   * @param[in] input JSON-formatted serialized topology, as detected by a remote topology manager
   */
  __USED__ inline void deserialize(const nlohmann::json &input)
  {
    // First, discard all existing information
    _deviceList.clear();

    // Sanity checks
    if (input.contains("Devices") == false) HICR_THROW_LOGIC("Serialized topology manager information is invalid, as it lacks the 'Devices' entry");
    if (input["Devices"].is_array() == false) HICR_THROW_LOGIC("Serialized topology manager 'Devices' entry is not an array.");

    for (auto device : input["Devices"])
    {
      if (device.contains("Type") == false) HICR_THROW_LOGIC("Serialized device information is invalid, as it lacks the 'Type' entry");
      if (device["Type"].is_string() == false) HICR_THROW_LOGIC("Serialized device information is invalid, as the 'Type' entry is not a string");
    }

    // Then call the backend-specific deserialization function
    deserializeImpl(input);

    // Checking whether the deserialization was successful
    if (_deviceList.size() != input["Devices"].size()) HICR_THROW_LOGIC("Deserialization failed, as the number of devices created (%lu) differs from the ones provided in the serialized input (%lu)", _deviceList.size(), input["Devices"].size());
  };

  /**
   * This function prompts the backend to perform the necessary steps to discover and list the compute units provided by the library which it supports.
   *
   * In case of change in resource availability during runtime, users need to re-run this function to be able to see the changes.
   *
   * \internal It does not return anything because we want to allow users to run only once, and then consult it many times without having to make a copy.
   */
  __USED__ inline void queryDevices()
  {
    // Clearing existing compute units
    _deviceList.clear();

    // Calling backend-internal implementation
    _deviceList = queryDevicesImpl();
  }

  /**
   * This function prompts the backend to perform the necessary steps to return  existing devices
   * \return A set of pointers to HiCR instances that refer to both local and remote instances
   */
  __USED__ inline const deviceList_t &getDevices() const { return _deviceList; }

  protected:

  /**
   * Backend-specific implementation of queryDevices
   *
   * \return The list of devices detected by this backend
   */
  virtual deviceList_t queryDevicesImpl() = 0;

  /**
   * Backend-specific implementation of the deserialize function
   *
   * @param[in] input Serialized topology information corresponding to the specific backend's topology manager
   */
  virtual void deserializeImpl(const nlohmann::json &input) = 0;

  /**
   * Map of devices queried by this topology manager
   */
  deviceList_t _deviceList;
};

} // namespace L1

} // namespace HiCR
