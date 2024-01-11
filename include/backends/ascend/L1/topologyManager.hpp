/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file topologyManager.hpp
 * @brief This file implements the TopologyManager class for the Ascend Backend
 * @author L. Terracciano & S. M. Martin
 * @date 18/12/2023
 */

#pragma once

#include <memory>
#include <backends/ascend/L0/device.hpp>
#include <backends/ascend/L0/computeResource.hpp>
#include <backends/ascend/L0/memorySpace.hpp>
#include <hicr/L0/topology.hpp>
#include <hicr/L1/topologyManager.hpp>

namespace HiCR
{

namespace backend
{

namespace ascend
{

namespace L1
{

/**
 * Implementation of the topology manager for the discovery and use of Ascend devices
 */
class TopologyManager final : public HiCR::L1::TopologyManager
{
  public:

  /**
   * Default constructor
   */
  TopologyManager() : HiCR::L1::TopologyManager() {}

  /**
   * Default destructor
   */
  ~TopologyManager() = default;

  __USED__ inline HiCR::L0::Topology queryTopology() override
  {
    // Storage for the topology to return
    HiCR::L0::Topology t;

    // Storage for device list
    std::unordered_set<std::shared_ptr<ascend::L0::Device>> ascendDeviceList;

    // Storage for getting the ascend device count
    uint32_t deviceCount = 0;

    // ask ACL for available devices
    aclError err;
    err = aclrtGetDeviceCount((uint32_t *)&deviceCount);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not retrieve ascend device count. Error %d", err);

    // add as many memory spaces as devices
    for (int32_t deviceId = 0; deviceId < (int32_t)deviceCount; deviceId++)
    {
      // Creating new devices
      size_t ascendFreeMemory, ascendMemorySize;
      auto deviceContext = new aclrtContext;

      // set the device
      err = aclrtSetDevice(deviceId);
      if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not select the ascend device %d. Error %d", deviceId, err);

      // retrieve the default device context
      err = aclrtGetCurrentContext(deviceContext);
      if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not get default context in ascend device %d. Error %d", deviceId, err);

      // get the memory info
      err = aclrtGetMemInfo(ACL_HBM_MEM, &ascendFreeMemory, &ascendMemorySize);
      if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not retrieve ascend device %d memory space. Error %d", deviceId, err);

      // Creating new Ascend device
      auto ascendDevice = std::make_shared<ascend::L0::Device>(deviceId, deviceContext, HiCR::L0::Device::computeResourceList_t({}), HiCR::L0::Device::memorySpaceList_t({}));

      // Creating Device's memory space
      auto ascendDeviceMemorySpace = std::make_shared<ascend::L0::MemorySpace>(ascendDevice, ascendMemorySize);

      // Creating Device's compute resource
      auto ascendDeviceComputeResource = std::make_shared<ascend::L0::ComputeResource>(ascendDevice);

      // Now adding resources to the device
      ascendDevice->addComputeResource(ascendDeviceComputeResource);
      ascendDevice->addMemorySpace(ascendDeviceMemorySpace);

      // Adding new device
      ascendDeviceList.insert(ascendDevice);
      t.addDevice(ascendDevice);
    }

    // Setting up communication between the local ascend devices
    setupInterDeviceCommunication(ascendDeviceList);

    // Returning topology
    return t;
  }

  /**
   * This function deserializes a JSON-encoded topology into a topology class with its constituent devices, as recognized by the called backend, and returns it
   *
   * If the backend does not recognize a device in the encoded topology, it will not add it to the topology
   *
   * @param[in] topology The JSON-encoded topology to deserialize
   * @return The deserialized topology containing only devices recognized by the backend
   */
  __USED__ static inline HiCR::L0::Topology deserializeTopology(const nlohmann::json &topology)
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
      if (type == "Ascend Device") t.addDevice(std::make_shared<ascend::L0::Device>(device));
    }

    // Returning new topology
    return t;
  }

  private:

  /**
   * Setup inter device communication in the ACL runtime.
   */
  __USED__ inline void setupInterDeviceCommunication(std::unordered_set<std::shared_ptr<ascend::L0::Device>> &ascendDeviceList)
  {
    int32_t canAccessPeer = 0;
    aclError err;

    // enable communication among each pair of ascend cards
    for (const auto &src : ascendDeviceList)
      for (const auto &dst : ascendDeviceList)
        if (src->getId() != dst->getId())
        {
          // verify that the two cards can see each other
          err = aclrtDeviceCanAccessPeer(&canAccessPeer, src->getId(), dst->getId());
          if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not determine peer accessibility to device %ld from device %ld.. Error %d", dst, src, err);

          if (canAccessPeer == 0) HICR_THROW_RUNTIME("Can not access device %ld from device %ld. Error %d", dst, src, err);

          // Selecting device
          ascend::L0::Device::selectDevice(dst->getContext(), dst->getId());

          // enable the communication
          err = aclrtDeviceEnablePeerAccess(src->getId(), 0);
          if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not enable peer access from device %ld to device %ld. Error %d", dst->getId(), src->getId(), err);
        }
  }

  /**
   * Hwloc implementation of the queryComputeResources() function. This will add one compute resource object per HW Thread / Processing Unit (PU) found
   */
  __USED__ inline HiCR::L0::Device::computeResourceList_t queryComputeResources()
  {
    // New compute resource list to return
    HiCR::L0::Device::computeResourceList_t computeResourceList;

    // Returning new compute resource list
    return computeResourceList;
  }

  /**
   * Hwloc implementation of the Backend queryMemorySpaces() function. This will add one memory space object per NUMA domain found
   */
  __USED__ inline HiCR::L0::Device::memorySpaceList_t queryMemorySpaces()
  {
    // New memory space list to return
    HiCR::L0::Device::memorySpaceList_t memorySpaceList;

    // Returning new memory space list
    return memorySpaceList;
  }
};

} // namespace L1

} // namespace ascend

} // namespace backend

} // namespace HiCR
