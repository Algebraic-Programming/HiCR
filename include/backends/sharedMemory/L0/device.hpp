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
#include <hicr/L0/device.hpp>
#include <hicr/L0/computeResource.hpp>
#include <hicr/L0/memorySpace.hpp>

namespace HiCR
{

namespace backend
{

namespace sharedMemory
{

namespace L0
{

/**
 * This class represents a device, as visible by the shared memory backend. That is, an assumed SMP processor plus a shared RAM that all process have access to.
 */
class Device final : public HiCR::L0::Device
{
  public:

  /**
   * Constructor for the device class of the sequential backend
   *
   * @param[in] computeResources The compute resources (cores or hyperthreads) detected in this device (CPU)
   * @param[in] memorySpaces The memory spaces (e.g., NUMA domains) detected in this device (CPU)
   */
  Device(const computeResourceList_t &computeResources, const memorySpaceList_t &memorySpaces) : HiCR::L0::Device(computeResources, memorySpaces){};

  /**
   * Default destructor
   */
  ~Device() = default;

  __USED__ inline std::string getType() const override { return "SMP Processor + Shared Memory"; }
};

} // namespace L0

} // namespace sharedMemory

} // namespace backend

} // namespace HiCR
