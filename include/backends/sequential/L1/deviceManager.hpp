/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file deviceManager.hpp
 * @brief This file implements support for device management of single processor systems
 * @author S. M. Martin
 * @date 18/12/2023
 */

#pragma once

#include <backends/sequential/L0/device.hpp>
#include <hicr/L1/deviceManager.hpp>

namespace HiCR
{

namespace backend
{

namespace sequential
{

namespace L1
{

/**
 * Implementation of the device manager for single processor host systems.
 */
class DeviceManager final : public HiCR::L1::DeviceManager
{
  public:

  /**
   * The constructor is employed to reserve memory required for hwloc
   */
  DeviceManager() : HiCR::L1::DeviceManager() {}

  /**
   * The constructor is employed to free memory required for hwloc
   */
  ~DeviceManager() = default;

  protected:

  __USED__ inline deviceList_t queryDevicesImpl()
  {
   // Creating single computing unit space representing a single core processor
   auto hostCPU = new sequential::L0::ComputeResource();

   // Creating single memory space representing host memory
   auto hostRam = new sequential::L0::MemorySpace();

   // Creating a single new device representing a single CPU plus host memory (RAM)
   auto hostDevice = new sequential::L0::Device({hostCPU}, {hostRam});

   // Returning single device
   return { hostDevice };
  }
};

} // namespace L1

} // namespace sequential

} // namespace backend

} // namespace HiCR
