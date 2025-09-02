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

/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file topologyManager.hpp
 * @brief This file implements the TopologyManager class for the acl Backend
 * @author L. Terracciano & S. M. Martin
 * @date 18/12/2023
 */

#pragma once

#include <memory>
#include <hicr/backends/acl/device.hpp>
#include <hicr/backends/acl/computeResource.hpp>
#include <hicr/backends/acl/memorySpace.hpp>
#include <hicr/core/topology.hpp>
#include <hicr/core/topologyManager.hpp>

namespace HiCR::backend::acl
{

/**
 * Implementation of the topology manager for the discovery and use of Huawei devices
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

    // Storage for getting the Huawei device count
    uint32_t deviceCount = 0;

    // ask ACL for available devices
    aclError err;
    err = aclrtGetDeviceCount((uint32_t *)&deviceCount);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not retrieve Huawei device count. Error %d", err);

    // add as many memory spaces as devices
    for (int32_t deviceId = 0; deviceId < (int32_t)deviceCount; deviceId++)
    {
      // Creating new devices
      size_t aclFreeMemory, aclMemorySize;

      // set the device
      err = aclrtSetDevice(deviceId);
      if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not select the Huawei device %d. Error %d", deviceId, err);

      // Creating new Huawei device
      auto aclDevice = std::make_shared<acl::Device>(deviceId, HiCR::Device::computeResourceList_t({}), HiCR::Device::memorySpaceList_t({}));

      // retrieve the default device context
      err = aclrtGetCurrentContext(aclDevice->getContext());
      if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not get default context in Huawei device %d. Error %d", deviceId, err);

      // get the memory info
      err = aclrtGetMemInfo(ACL_HBM_MEM, &aclFreeMemory, &aclMemorySize);
      if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not retrieve Huawei device %d memory space. Error %d", deviceId, err);

      // Creating Device's memory space
      auto aclDeviceMemorySpace = std::make_shared<acl::MemorySpace>(aclDevice, aclMemorySize);

      // Creating Device's compute resource
      auto aclDeviceComputeResource = std::make_shared<acl::ComputeResource>(aclDevice);

      // Now adding resources to the device
      aclDevice->addComputeResource(aclDeviceComputeResource);
      aclDevice->addMemorySpace(aclDeviceMemorySpace);

      // Adding new device
      t.addDevice(aclDevice);
    }

    // Setting up communication between the local Huawei devices
    setupInterDeviceCommunication(t);

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
      if (type == "Huawei Device") t.addDevice(std::make_shared<acl::Device>(device));
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
    // Initialize ACL runtime
    aclError err = aclInit(NULL);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Failed to initialize acl. Error %d", err);

    // Initializing acl topology manager
    return std::make_unique<HiCR::backend::acl::TopologyManager>();
  }

  private:

  /**
   * Setup inter device communication in the ACL runtime.
   */
  __INLINE__ void setupInterDeviceCommunication(HiCR::Topology &topology)
  {
    int32_t  canAccessPeer = 0;
    aclError err;

    // enable communication among each pair of Huawei devices
    for (const auto &srcDevice : topology.getDevices())
      for (const auto &dstDevice : topology.getDevices())
      {
        auto src = (acl::Device *)srcDevice.get();
        auto dst = (acl::Device *)dstDevice.get();
        if (src->getId() != dst->getId())
        {
          // verify that the two Huawei devices can see each other
          err = aclrtDeviceCanAccessPeer(&canAccessPeer, src->getId(), dst->getId());
          if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not determine peer accessibility to device %ld from device %ld.. Error %d", dst, src, err);

          if (canAccessPeer == 0) HICR_THROW_RUNTIME("Can not access device %ld from device %ld. Error %d", dst, src, err);

          // Selecting device
          dst->select();

          // enable the communication
          err = aclrtDeviceEnablePeerAccess(src->getId(), 0);
          if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not enable peer access from device %ld to device %ld. Error %d", dst->getId(), src->getId(), err);
        }
      }
  }

  /**
   * Hwloc implementation of the queryComputeResources() function. This will add one compute resource object per HW Thread / Processing Unit (PU) found
   */
  __INLINE__ HiCR::Device::computeResourceList_t queryComputeResources()
  {
    // New compute resource list to return
    HiCR::Device::computeResourceList_t computeResourceList;

    // Returning new compute resource list
    return computeResourceList;
  }

  /**
   * Hwloc implementation of the Backend queryMemorySpaces() function. This will add one memory space object per NUMA domain found
   */
  __INLINE__ HiCR::Device::memorySpaceList_t queryMemorySpaces()
  {
    // New memory space list to return
    HiCR::Device::memorySpaceList_t memorySpaceList;

    // Returning new memory space list
    return memorySpaceList;
  }
};

} // namespace HiCR::backend::acl
