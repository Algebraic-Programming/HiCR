/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file device.hpp
 * @brief This file implements the Device class for the sequential backend
 * @author S. M. Martin
 * @date 18/12/2023
 */

#pragma once

#include <hicr/L0/device.hpp>
#include <backends/sequential/L0/computeResource.hpp>
#include <backends/sequential/L0/memorySpace.hpp>
#include <hicr/common/definitions.hpp>

namespace HiCR
{

namespace backend
{

namespace sequential
{

namespace L0
{

/** 
 * This class represents a device, as visible by the sequential backend. That is, an assumed single core processor plus the entire RAM that the CPU has access to.
 */
class Device final : public HiCR::L0::Device
{
  public:

  /**
   * Constructor for the device class of the sequential backend
   * 
   * \param[in] computeResources The compute resources associated to this device (just one, the CPU as a whole)
   * \param[in] memorySpaces The memory spaces associated to this device (just one, the host RAM)
   */
  Device(const computeResourceList_t& computeResources, const memorySpaceList_t& memorySpaces) : HiCR::L0::Device(computeResources, memorySpaces) {};

  /**
   * Default destructor
   */
  ~Device() = default;

  __USED__ inline std::string getType() const override { return "Host Device"; }
};

} // namespace L0

} // namespace sequential

} // namespace backend

} // namespace HiCR
