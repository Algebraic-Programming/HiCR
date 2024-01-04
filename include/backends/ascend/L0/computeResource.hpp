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

#include <hicr/definitions.hpp>
#include <hicr/exceptions.hpp>
#include <hicr/L0/computeResource.hpp>

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
   *
   * \param device The Ascend device that contains this compute resource
   */
  ComputeResource(const std::shared_ptr<const ascend::L0::Device> device) : HiCR::L0::ComputeResource(),
                                                      _device(device){};

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
  __USED__ inline const std::weak_ptr<const ascend::L0::Device> getDevice() const
  {
    return _device;
  }

  private:

  /**
   * Stores the device that owns this compute resource
   */
  const std::weak_ptr<const ascend::L0::Device> _device;
};

} // namespace L0

} // namespace ascend

} // namespace backend

} // namespace HiCR
