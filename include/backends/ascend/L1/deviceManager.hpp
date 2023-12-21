/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file deviceManager.hpp
 * @brief This file implements the DeviceManager class for the Ascend Backend
 * @author L. Terracciano & S. M. Martin
 * @date 18/12/2023
 */

#pragma once

#include <backends/ascend/L0/device.hpp>
#include <backends/ascend/L0/computeResource.hpp>
#include <backends/ascend/L0/memorySpace.hpp>
#include <hicr/L1/deviceManager.hpp>

namespace HiCR
{

namespace backend
{

namespace ascend
{

namespace L1
{

/**
 * Implementation of the device manager for the discovery and use of Ascend devices
 */
class DeviceManager final : public HiCR::L1::DeviceManager
{
  public:

  /**
   * Default constructor
   */
  DeviceManager() : HiCR::L1::DeviceManager() {}

  /**
   * Default destructor
   */
  ~DeviceManager() = default;

  protected:

  __USED__ inline deviceList_t queryDevicesImpl()
  {
    // Storage for device list
    std::unordered_set<ascend::L0::Device*> ascendDeviceList;
    std::unordered_set<HiCR::L0::Device*> HiCRDeviceList;

    // Storage for getting the ascend device count
    uint32_t deviceCount = 0;

    // ask ACL for available devices
    aclError err;
    err = aclrtGetDeviceCount((uint32_t *)&deviceCount);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not retrieve ascend device count. Error %d", err);

    // Creating new devices
    size_t ascendFreeMemory, ascendMemorySize;
    aclrtContext deviceContext;

    // add as many memory spaces as devices
    for (int32_t deviceId = 0; deviceId < (int32_t)deviceCount; deviceId++)
    {
      // set the device
      err = aclrtSetDevice(deviceId);
      if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not select the ascend device %d. Error %d", deviceId, err);

      // retrieve the default device context
      err = aclrtGetCurrentContext(&deviceContext);
      if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not get default context in ascend device %d. Error %d", deviceId, err);

      // get the memory info
      err = aclrtGetMemInfo(ACL_HBM_MEM, &ascendFreeMemory, &ascendMemorySize);
      if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not retrieve ascend device %d memory space. Error %d", deviceId, err);

      // Creating Device's memory space
      auto ascendDeviceMemorySpace = new ascend::L0::MemorySpace(ascendMemorySize);

      // Creating Device's compute resource
      auto ascendDeviceComputeResource = new ascend::L0::ComputeResource;

      // Creating new Ascend device
      auto ascendDevice = new ascend::L0::Device(deviceId, deviceContext, {ascendDeviceComputeResource}, {ascendDeviceMemorySpace});

      // Setting backward pointers to the memory space and compute resources. This circular reference is not ideal, but is the only solution so far
      ascendDeviceMemorySpace->setDevice(ascendDevice);
      ascendDeviceComputeResource->setDevice(ascendDevice);

      // Adding new device
      ascendDeviceList.insert(ascendDevice);
      HiCRDeviceList.insert(ascendDevice);
    }

   // Setting up communication between the local ascend devices
   setupInterDeviceCommunication(ascendDeviceList);

   // Returning device list
   return HiCRDeviceList;
  }

private:

/**
 * Setup inter device communication in the ACL runtime.
 */
__USED__ inline void setupInterDeviceCommunication(std::unordered_set<ascend::L0::Device*>& ascendDeviceList)
{
  int32_t canAccessPeer = 0;
  aclError err;

  // enable communication among each pair of ascend cards
  for (const auto src : ascendDeviceList) 
    for (const auto dst : ascendDeviceList)
     if (src->getId() != dst->getId())
      {
        // verify that the two cards can see each other
        err = aclrtDeviceCanAccessPeer(&canAccessPeer, src->getId(), dst->getId());
        if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not determine peer accessibility to device %ld from device %ld.. Error %d", dst, src, err);

        if (canAccessPeer == 0) HICR_THROW_RUNTIME("Can not access device %ld from device %ld. Error %d", dst, src, err);

        selectDevice(dst->getContext(), dst->getId());

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
