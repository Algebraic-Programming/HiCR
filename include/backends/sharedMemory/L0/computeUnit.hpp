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

#include <hicr/L0/computeUnit.hpp>
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
class ComputeUnit final : public HiCR::L0::ComputeUnit
{
  public:

  /**
   * Constructor for the execution unit class of the sequential backend
   *
   * \param fc A replicable C++ function to run in this execution unit
   */
  ComputeUnit(const int affinity) : HiCR::L0::ComputeUnit(), _affinity(affinity) {};
  ComputeUnit() = delete;

  /**
   * Default destructor
   */
  ~ComputeUnit() = default;

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
