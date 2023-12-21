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
 * Forward declaration of the ascend device class -- a not-so-elegant solution to a circular dependency, but all we can do for now
*/
class Device;

/**
 * This class represents a compute resource, visible by the sequential backend. That is, a CPU processing unit (core or hyperthread) with information about caches and locality.
 */
class ComputeResource final : public HiCR::L0::ComputeResource
{
  public:

  /**
   * Constructor for the compute resource class of the sequential backend
   */
  ComputeResource() :
   HiCR::L0::ComputeResource() {};

  /**
   * Default destructor
   */
  ~ComputeResource() = default;

  __USED__ inline std::string getType() const override { return "Ascend Processor"; }

  /**
  * Function to get the device id associated to this memory space
  *
  * @return The device id corresponding to this memory space
  */
  __USED__ inline const ascend::L0::Device* getDevice() const
  {
    return _device;
  }

  /**
  * Function to set the ascend device associated to this memory space
  */
  __USED__ inline void setDevice(const ascend::L0::Device* device)  { _device = device; }


  private:

  /**
   * Stores the device that owns this compute resource
  */
  const ascend::L0::Device* _device;
};

} // namespace L0

} // namespace ascend

} // namespace backend

} // namespace HiCR
