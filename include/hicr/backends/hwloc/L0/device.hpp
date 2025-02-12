/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file device.hpp
 * @brief This file implements the Device class for the  HWLoc-based backend
 * @author S. M. Martin
 * @date 18/12/2023
 */

#pragma once

#include <hicr/core/definitions.hpp>
#include <hicr/core/L0/device.hpp>
#include "computeResource.hpp"
#include "memorySpace.hpp"

namespace HiCR::backend::hwloc::L0
{

/**
 * This class represents a device, as visible by the shared memory backend. That is, an assumed SMP processor plus a shared RAM that all process have access to.
 */
class Device final : public HiCR::L0::Device
{
  public:

  /**
   * Type definition for a NUMA Domain identifier
   */
  using NUMADomainID_t = unsigned int;

  /**
   * Constructor for the device class of the sequential backend
   *
   * @param[in] NUMADomainId The OS-given NUMA domain identifier represented by this class
   * @param[in] computeResources The compute resources (cores or hyperthreads) detected in this device (CPU)
   * @param[in] memorySpaces The memory spaces (e.g., NUMA domains) detected in this device (CPU)
   */
  Device(const NUMADomainID_t NUMADomainId, const computeResourceList_t &computeResources, const memorySpaceList_t &memorySpaces)
    : HiCR::L0::Device(computeResources, memorySpaces),
      _NUMADomainId(NUMADomainId){};

  /**
   * Empty constructor for serialization / deserialization
   */
  Device()
    : HiCR::L0::Device(){};

  /**
   * Deserializing constructor
   *
   * The instance created will contain all information, if successful in deserializing it, corresponding to the passed NUMA domain
   * This instance should NOT be used for anything else than reporting/printing the contained resources
   *
   * @param[in] input A JSON-encoded serialized NUMA domain information
   */
  Device(const nlohmann::json &input)
    : HiCR::L0::Device()
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
    for (const auto &computeResource : input["Compute Resources"])
    {
      // Getting device type
      const auto type = computeResource["Type"].get<std::string>();

      // Checking whether the type is correct
      if (type != "Processing Unit") HICR_THROW_LOGIC("The passed device type '%s' is not compatible with this topology manager", type.c_str());

      // Deserializing new device
      auto computeResourceObj = std::make_shared<hwloc::L0::ComputeResource>(computeResource);

      // Inserting device into the list
      this->addComputeResource(computeResourceObj);
    }

    // Iterating over the memory space list
    for (const auto &memorySpace : input["Memory Spaces"])
    {
      // Getting device type
      const auto type = memorySpace["Type"].get<std::string>();

      // Checking whether the type is correct
      if (type != "RAM") HICR_THROW_LOGIC("The passed device type '%s' is not compatible with this topology manager", type.c_str());

      // Deserializing new device
      auto memorySpaceObj = std::make_shared<hwloc::L0::MemorySpace>(memorySpace);

      // Inserting device into the list
      this->addMemorySpace(memorySpaceObj);
    }
  }
};

} // namespace HiCR::backend::hwloc::L0
