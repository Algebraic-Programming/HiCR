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
 * @file topologyManager.hpp
 * @brief This file implements the TopologyManager class for the OpenCL Backend
 * @author L. Terracciano
 * @date 06/03/2025
 */

#pragma once

#include <memory>
#include <hicr/backends/opencl/device.hpp>
#include <hicr/backends/opencl/computeResource.hpp>
#include <hicr/backends/opencl/memorySpace.hpp>
#include <hicr/core/topology.hpp>
#include <hicr/core/topologyManager.hpp>

namespace HiCR::backend::opencl
{
/**
 * Implementation of the topology manager for the discovery and use of OpenCL devices
 */
class TopologyManager final : public HiCR::TopologyManager
{
  public:

  /**
   * Default constructor
   */
  TopologyManager()
    : HiCR::TopologyManager()
  {}

  /**
   * Default destructor
   */
  ~TopologyManager() = default;

  __INLINE__ HiCR::Topology queryTopology() override
  {
    // Storage for the topology to return
    HiCR::Topology t;

    // Get available platforms
    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);

    if (platforms.empty()) { HICR_THROW_RUNTIME("No devices found"); }

    for (const auto &platform : platforms)
    {
      // Retrieve all kind of devices detected by OpenCL
      std::vector<cl::Device> devices;
      platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);

      opencl::Device::deviceIdentifier_t deviceId = 0;

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
        auto openclDevice = std::make_shared<opencl::Device>(
          deviceId, deviceType, std::make_shared<cl::Device>(device), HiCR::Device::computeResourceList_t({}), HiCR::Device::memorySpaceList_t({}));
        auto openclDeviceMemorySpace     = std::make_shared<opencl::MemorySpace>(openclDevice, deviceType + " RAM", deviceMemorySize);
        auto openclDeviceComputeResource = std::make_shared<opencl::ComputeResource>(openclDevice, deviceType + " Processing Unit");
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
  __INLINE__ static HiCR::Topology deserializeTopology(const nlohmann::json &topology)
  {
    // Verifying input's syntax
    HiCR::Topology::verify(topology);

    // New topology to create
    HiCR::Topology t;

    // Iterating over the device list entries in the serialized input
    for (const auto &device : topology["Devices"])
    {
      // Getting device type
      const auto type = device["Type"].get<std::string>();

      // If the device type is recognized, add it to the new topology
      if (type.find("OpenCL") != std::string::npos) { t.addDevice(std::make_shared<opencl::Device>(device)); }
    }

    // Returning new topology
    return t;
  }

  __INLINE__ HiCR::Topology _deserializeTopology(const nlohmann::json &topology) const override { return deserializeTopology(topology); }

  /**
   * This function represents the default intializer for this backend
   *
   * @return A unique pointer to the newly instantiated backend class
   */
  __INLINE__ static std::unique_ptr<HiCR::TopologyManager> createDefault()
  {
    // Initializing opencl topology manager
    return std::make_unique<HiCR::backend::opencl::TopologyManager>();
  }

  private:

  /**
   * OpenCL implementation of the queryComputeResources() function. This will add one compute resource object per HW Thread / Processing Unit (PU) found
   */
  __INLINE__ HiCR::Device::computeResourceList_t queryComputeResources()
  {
    // New compute resource list to return
    HiCR::Device::computeResourceList_t computeResourceList;

    // Returning new compute resource list
    return computeResourceList;
  }

  /**
   * OpenCL implementation of the Backend queryMemorySpaces() function. This will add one memory space object per NUMA domain found
   */
  __INLINE__ HiCR::Device::memorySpaceList_t queryMemorySpaces()
  {
    // New memory space list to return
    HiCR::Device::memorySpaceList_t memorySpaceList;

    // Returning new memory space list
    return memorySpaceList;
  }
};

} // namespace HiCR::backend::opencl