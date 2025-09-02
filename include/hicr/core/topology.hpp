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
 * @brief Provides a base definition for a HiCR Topology class
 * @author S. M. Martin
 * @date 18/12/2023
 */
#pragma once

#include <memory>
#include <unordered_set>
#include <nlohmann_json/json.hpp>
#include <nlohmann_json/parser.hpp>
#include <hicr/core/device.hpp>

namespace HiCR
{

/**
 * This class represents an abstract definition for a HiCR Topology that:
 *
 * - Describes the physical resources (devices) of a given system (real or constructed for requesting new currents)
 * - Information about the connectivity between the given devices
 */
class Topology
{
  public:

  /**
   * Common type for a collection of devices
   */
  using deviceList_t = std::vector<std::shared_ptr<Device>>;

  Topology()  = default;
  ~Topology() = default;

  /**
   * Deserializing constructor
   *
   * @param[in] input A JSON-encoded serialized topology information
   */
  Topology(const nlohmann::json &input) { deserialize(input); }

  /**
   * This function prompts the backend to perform the necessary steps to return  existing devices
   * \return A set of pointers to HiCR currents that refer to both local and remote currents
   */
  [[nodiscard]] __INLINE__ const deviceList_t &getDevices() const { return _deviceList; }

  /**
   * This function allows manually adding a new device into an existing topology
   *
   * @param[in] device The device to add
   */
  __INLINE__ void addDevice(const std::shared_ptr<HiCR::Device> &device) { _deviceList.push_back(device); }

  /**
   * This function allows manually merging one topology information into another
   *
   * @param[in] source The source topology to add into this current
   */
  __INLINE__ void merge(const Topology &source)
  {
    // Adding each device separately
    for (const auto &d : source.getDevices()) addDevice(d);
  }

  /**
   * Serialization function to enable sharing topology information across different HiCR currents (or any other purposes)
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
   * De-serialization function to re-construct the serialized topology information coming (typically) from remote instances
   *
   * @param[in] input JSON-formatted serialized device information
   *
   * \note Important: Deserialized devices are not meant to be used in any from other than printing or reporting its topology.
   *       Any attempt of actually using them for computation or data transfers will result in undefined behavior.
   */
  __INLINE__ void deserialize(const nlohmann::json &input)
  {
    if (input.contains("Devices"))
    {
      const auto &devicesJs = hicr::json::getArray<nlohmann::json>(input, "Devices");
      for (const auto &deviceJs : devicesJs) addDevice(std::make_shared<Device>(deviceJs));
    }

    if (input.contains("Metadata")) _metadata = input["Metadata"];
  };

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
  * Checks whether the given topology 2 is a subset of a given topology 1
  * That is, whether it contains the given devices in the current type provided
  *
  * The devices are checked in order. That is the first current device that satisfies a given device
  * will be removed from the list when checking the next given device.
  * 
  * @param[in] topology1 The topology proposed as superset
  * @param[in] topology2 The topology proposed as subset
  * 
  * @return true, if topology2 is a subset of topology 1; false, otherwise.
  */
  [[nodiscard]] static __INLINE__ bool isSubset(const HiCR::Topology &topology1, const HiCR::Topology &topology2)
  {
    // Making a copy of the current topology.
    // Devices will be removed as we match them with the given device
    auto devices1 = topology1.getDevices();

    ////////// Checking for given devices
    const auto devices2 = topology2.getDevices();

    for (const auto &device2 : devices2)
    {
      const auto device2Type             = device2->getType();
      const auto device2MemorySpaces     = device2->getMemorySpaceList();
      const auto device2ComputeResources = device2->getComputeResourceList();

      // Iterating over all the current devices to see if one of them satisfies this given device
      bool foundCompatibleDevice = false;
      for (auto device1Itr = devices1.begin(); device1Itr != devices1.end() && foundCompatibleDevice == false; device1Itr++)
      {
        // Getting current device object
        const auto &device1 = device1Itr.operator*();

        // Checking type
        const auto &device1Type = device1->getType();

        // If this device is the same type as given, proceed to check compute resources and memory spaces
        if (device1Type == device2Type)
        {
          // Flag to indicate this device is compatible with the request
          bool deviceIsCompatible = true;

          ///// Checking given compute resources
          auto device1ComputeResources = device1->getComputeResourceList();

          // Getting compute resources in this current device
          for (const auto &givenComputeResource : device2ComputeResources)
          {
            bool foundComputeResource = false;
            for (auto device1ComputeResourceItr = device1ComputeResources.begin(); device1ComputeResourceItr != device1ComputeResources.end() && foundComputeResource == false;
                 device1ComputeResourceItr++)
            {
              // Getting current device object
              const auto &device1ComputeResource = device1ComputeResourceItr.operator*();

              // If it's the same type as given
              if (device1ComputeResource->getType() == givenComputeResource->getType())
              {
                // Set compute resource as found
                foundComputeResource = true;

                // Erase this element from the list to not re-use it
                device1ComputeResources.erase(device1ComputeResourceItr);
              }
            }

            // If no compute resource was found, abandon search in this device
            if (foundComputeResource == false)
            {
              deviceIsCompatible = false;
              break;
            }
          }

          // If no suitable device was found, advance with the next one
          if (deviceIsCompatible == false) continue;

          ///// Checking given compute resources
          auto device1MemorySpaces = device1->getMemorySpaceList();

          // Getting compute resources in this current device
          for (const auto &device2MemorySpace : device2MemorySpaces)
          {
            bool foundMemorySpace = false;
            for (auto device1MemorySpaceItr = device1MemorySpaces.begin(); device1MemorySpaceItr != device1MemorySpaces.end() && foundMemorySpace == false; device1MemorySpaceItr++)
            {
              // Getting current device object
              const auto &device1MemorySpace = device1MemorySpaceItr.operator*();

              // If it's the same type as given
              if (device1MemorySpace->getType() == device2MemorySpace->getType())
              {
                // Check whether the size is at least as big as given
                if (device1MemorySpace->getSize() >= device2MemorySpace->getSize())
                {
                  // Set compute resource as found
                  foundMemorySpace = true;

                  // Erase this element from the list to not re-use it
                  device1MemorySpaces.erase(device1MemorySpaceItr);
                }
              }
            }

            // If no compute resource was found, abandon search in this device
            if (foundMemorySpace == false)
            {
              deviceIsCompatible = false;
              break;
            }
          }

          // If no suitable device was found, advance with the next one
          if (deviceIsCompatible == false) continue;

          // If we reached this point, we've found the device
          foundCompatibleDevice = true;

          // Deleting device to prevent it from being counted again
          devices1.erase(device1Itr);

          // Stop checking
          break;
        }
      }

      // If no current devices could satisfy the given device, return false now
      if (foundCompatibleDevice == false) return false;
    }

    // All requirements have been met, returning true
    //printf("Requirements met\n");
    return true;
  }

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

} // namespace HiCR
