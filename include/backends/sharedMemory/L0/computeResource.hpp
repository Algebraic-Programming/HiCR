/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file executionUnit.hpp
 * @brief This file implements the compute unit class for the sequential backend
 * @author S. M. Martin
 * @date 12/12/2023
 */

#pragma once

#include <hicr/L0/computeResource.hpp>
#include <hicr/common/definitions.hpp>

namespace HiCR
{

namespace backend
{

namespace sharedMemory
{

namespace L0
{

/**
 * This class represents a compute resource, visible by the sequential backend. That is, a CPU core with information about locality.
 */
class ComputeResource final : public HiCR::L0::ComputeResource
{
  public:

  /**
   * Constructor for the compute resource class of the sequential backend
   *
   * \param affinity Os-determied core affinity assigned to this compute resource
   */
  ComputeResource(const int affinity) : HiCR::L0::ComputeResource(), _affinity(affinity) {};
  ComputeResource() = delete;

  /**
   * Default destructor
   */
  ~ComputeResource() = default;

  __USED__ inline std::string getType() const override { return "CPU Core"; }

  /**
   * Function to return the core's affinity
   * 
   * @returns The core's affinity
  */
  __USED__ inline int getAffinity() const { return _affinity; }

  private:

  const int _affinity;
};

} // namespace L0

} // namespace sharedMemory

} // namespace backend

} // namespace HiCR
