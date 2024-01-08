/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file device.hpp
 * @brief This file implements the Device class for the shared memory backend
 * @author S. M. Martin
 * @date 18/12/2023
 */

#pragma once

#include <hicr/definitions.hpp>
#include <backends/sharedMemory/L0/device.hpp>
#include <backends/sharedMemory/hwloc/L0/computeResource.hpp>
#include <backends/sharedMemory/hwloc/L0/memorySpace.hpp>

namespace HiCR
{

namespace backend
{

namespace sharedMemory
{

namespace hwloc
{

namespace L0
{

/**
 * This class represents a device, as visible by the shared memory backend. That is, an assumed SMP processor plus a shared RAM that all process have access to.
 */
class Device final : public HiCR::backend::sharedMemory::L0::Device
{
  public:

  /**
   * Type definition for a NUMA Domain identifier
  */
  typedef int NUMADomainID_t; 

  /**
   * Constructor for the device class of the sequential backend
   *
   * @param[in] NUMADomainId The OS-given NUMA domain identifier represented by this class
   * @param[in] computeResources The compute resources (cores or hyperthreads) detected in this device (CPU)
   * @param[in] memorySpaces The memory spaces (e.g., NUMA domains) detected in this device (CPU)
   */
  Device(const NUMADomainID_t NUMADomainId, const computeResourceList_t &computeResources, const memorySpaceList_t &memorySpaces) : 
  HiCR::backend::sharedMemory::L0::Device(NUMADomainId, computeResources, memorySpaces) {};

  /**
   * Deserializing constructor
  */
  Device(const nlohmann::json& input) : HiCR::backend::sharedMemory::L0::Device()
  {
    deserialize(input);
  }

  /**
   * Default destructor
   */
  ~Device() = default;

  private:

  __USED__ inline void serializeImpl(nlohmann::json& output) const override
  {
    // Nothing extra to serialize here
  }
  
  __USED__ inline void deserializeImpl(const nlohmann::json& input) override
  {
    // Iterating over the compute resource list
    for (const auto& computeResource : input["Compute Resources"])
    {
      // Getting device type
      const auto type = computeResource["Type"].get<std::string>();

      // Checking whether the type is correct
      if (type != "CPU Core") HICR_THROW_LOGIC("The passed device type '%s' is not compatible with this topology manager", type.c_str());   

      // Deserializing new device
      auto computeResourceObj = std::make_shared<sharedMemory::hwloc::L0::ComputeResource>(computeResource);
      
      // Inserting device into the list
      _computeResources.insert(computeResourceObj);
    }

    // Iterating over the memory space list
    for (const auto& memorySpace : input["Memory Spaces"])
    {
      // Getting device type
      const auto type = memorySpace["Type"].get<std::string>();

      // Checking whether the type is correct
      if (type != "Host RAM") HICR_THROW_LOGIC("The passed device type '%s' is not compatible with this topology manager", type.c_str());   

      // Deserializing new device
      auto memorySpaceObj = std::make_shared<sharedMemory::hwloc::L0::MemorySpace>(memorySpace);
      
      // Inserting device into the list
      _memorySpaces.insert(memorySpaceObj);
    }
  }
};

} // namespace L0

} // namespace hwloc

} // namespace sharedMemory

} // namespace backend

} // namespace HiCR
