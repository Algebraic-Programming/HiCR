/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file topologyManager.hpp
 * @brief This file implements the TopologyManager class for the OpenCL Backend
 * @author L. Terracciano
 * @date 06/03/2025
 */

#pragma once

#include <memory>
#include <hicr/backends/opencl/L0/device.hpp>
#include <hicr/backends/opencl/L0/computeResource.hpp>
#include <hicr/backends/opencl/L0/memorySpace.hpp>
#include <hicr/core/L0/topology.hpp>
#include <hicr/core/L1/topologyManager.hpp>

namespace HiCR
{

namespace backend
{

namespace opencl
{

namespace L1
{

/**
 * Implementation of the topology manager for the discovery and use of OpenCL devices
 */
class TopologyManager final : public HiCR::L1::TopologyManager
{
  public:

  /**
   * Default constructor
   */
  TopologyManager()
    : HiCR::L1::TopologyManager()
  {}

  /**
   * Default destructor
   */
  ~TopologyManager() = default;

  __INLINE__ HiCR::L0::Topology queryTopology() override
  {
    // Storage for the topology to return
    HiCR::L0::Topology t;

    // Get available platforms
    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);

    if (platforms.empty()) { HICR_THROW_RUNTIME("No devices found"); }

    for (const auto &platform : platforms)
    {
      // Retrieve all kind of devices detected by OpenCL
      std::vector<cl::Device> devices;
      platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);

      opencl::L0::Device::deviceIdentifier_t deviceId = 0;

      for (const auto &device : devices)
      {
        // Get device type and its memory size
        auto deviceTypeEnum   = device.getInfo<CL_DEVICE_TYPE>();
        auto deviceMemorySize = device.getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>();

        std::string deviceType;
        switch (deviceTypeEnum)
        {
        case CL_DEVICE_TYPE_CPU: deviceType = "OpenCL Host"; break;
        case CL_DEVICE_TYPE_GPU: deviceType = "OpenCL GPU"; break;
        case CL_DEVICE_TYPE_ACCELERATOR: deviceType = "OpenCL Accelerator"; break;
        case CL_DEVICE_TYPE_CUSTOM: deviceType = "OpenCL Custom Hardware"; break;
        default: HICR_THROW_RUNTIME("Unsupported device type: %lu", deviceTypeEnum); break;
        }

        // Create HiCR device, memory space, and compute resource
        auto openclDevice = std::make_shared<opencl::L0::Device>(
          deviceId, deviceType, std::make_shared<cl::Device>(device), HiCR::L0::Device::computeResourceList_t({}), HiCR::L0::Device::memorySpaceList_t({}));
        auto openclDeviceMemorySpace     = std::make_shared<opencl::L0::MemorySpace>(openclDevice, deviceType + " RAM", deviceMemorySize);
        auto openclDeviceComputeResource = std::make_shared<opencl::L0::ComputeResource>(openclDevice, deviceType + " Processing Unit");
        openclDevice->addMemorySpace(openclDeviceMemorySpace);
        openclDevice->addComputeResource(openclDeviceComputeResource);

        t.addDevice(openclDevice);
      }
    }

    // Returning topology
    return t;
  }

  /**
   * Static implementation of the _deserializeTopology Function
   * @param[in] topology see: _deserializeTopology
   * @return see: _deserializeTopology
   */
  __INLINE__ static HiCR::L0::Topology deserializeTopology(const nlohmann::json &topology)
  {
    // Verifying input's syntax
    HiCR::L0::Topology::verify(topology);

    // New topology to create
    HiCR::L0::Topology t;

    // Iterating over the device list entries in the serialized input
    for (const auto &device : topology["Devices"])
    {
      // Getting device type
      const auto type = device["Type"].get<std::string>();

      // If the device type is recognized, add it to the new topology
      if (type.find("OpenCL") != std::string::npos) t.addDevice(std::make_shared<opencl::L0::Device>(device));
    }

    // Returning new topology
    return t;
  }

  __INLINE__ HiCR::L0::Topology _deserializeTopology(const nlohmann::json &topology) const override { return deserializeTopology(topology); }

  /**
   * This function represents the default intializer for this backend
   *
   * @return A unique pointer to the newly instantiated backend class
   */
  __INLINE__ static std::unique_ptr<HiCR::L1::TopologyManager> createDefault()
  {
    // Initializing opencl topology manager
    return std::make_unique<HiCR::backend::opencl::L1::TopologyManager>();
  }

  private:

  /**
   * OpenCL implementation of the queryComputeResources() function. This will add one compute resource object per HW Thread / Processing Unit (PU) found
   */
  __INLINE__ HiCR::L0::Device::computeResourceList_t queryComputeResources()
  {
    // New compute resource list to return
    HiCR::L0::Device::computeResourceList_t computeResourceList;

    // Returning new compute resource list
    return computeResourceList;
  }

  /**
   * OpenCL implementation of the Backend queryMemorySpaces() function. This will add one memory space object per NUMA domain found
   */
  __INLINE__ HiCR::L0::Device::memorySpaceList_t queryMemorySpaces()
  {
    // New memory space list to return
    HiCR::L0::Device::memorySpaceList_t memorySpaceList;

    // Returning new memory space list
    return memorySpaceList;
  }
};

} // namespace L1

} // namespace opencl

} // namespace backend

} // namespace HiCR
