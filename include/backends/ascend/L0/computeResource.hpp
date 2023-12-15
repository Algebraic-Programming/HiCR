/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file computeResource.hpp
 * @brief This file implements the compute resource class for the Ascend backend
 * @author L. Terracciano & S. M. Martin
 * @date 15/12/2023
 */

#pragma once

#include "hwloc.h"
#include <backends/ascend/common.hpp>
#include <hicr/L0/computeResource.hpp>
#include <hicr/common/definitions.hpp>
#include <hicr/common/exceptions.hpp>

namespace HiCR
{

namespace backend
{

namespace ascend
{

namespace L0
{

/**
 * This class represents a compute resource, visible by the sequential backend. That is, a CPU processing unit (core or hyperthread) with information about caches and locality.
 */
class ComputeResource final : public HiCR::L0::ComputeResource
{
  public:

  /**
   * Constructor for the compute resource class of the sequential backend
   */
  ComputeResource(
   const ascend::deviceType_t deviceType,
   const ascend::deviceIdentifier_t deviceId) :
   HiCR::L0::ComputeResource(),
   _deviceType(deviceType),
   _deviceId(deviceId)
     {};

  /**
   * Default destructor
   */
  ~ComputeResource() = default;

  __USED__ inline std::string getType() const override { return "Ascend Processor"; }


  /**
   * Function to get the device type associated to this memory space
   *
   * @return The device type corresponding to this memory space
   */
  __USED__ inline const ascend::deviceType_t getDeviceType() const
  {
    return _deviceType;
  }

  /**
  * Function to get the device id associated to this memory space
  *
  * @return The device id corresponding to this memory space
  */
  __USED__ inline const ascend::deviceIdentifier_t getDeviceId() const
  {
    return _deviceId;
  }

  private:

  /**
   * Stores the device type that this memory space belongs to
   */
  const ascend::deviceType_t _deviceType;

  /**
   * Stores the device identifier that hold this memory space. This might need to be removed in favor of defining
   * a device class
  */
 const ascend::deviceIdentifier_t _deviceId;
};

} // namespace L0

} // namespace ascend

} // namespace backend

} // namespace HiCR
