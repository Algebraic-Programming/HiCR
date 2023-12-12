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

namespace sequential
{

namespace L0
{

/**
 * This class represents a compute resource, visible by the sequential backend. That is, a CPU processor with no information about locality and no assumption of multi-processing support.
 */
class ComputeUnit final : public HiCR::L0::ComputeUnit
{
  public:

  /**
   * Constructor for the execution unit class of the sequential backend
   *
   * \param fc A replicable C++ function to run in this execution unit
   */
  ComputeUnit() : HiCR::L0::ComputeUnit() {};

  /**
   * Default destructor
   */
  ~ComputeUnit() = default;

  __USED__ inline std::string getType() const override { return "CPU Processor"; }

};

} // namespace L0

} // namespace sequential

} // namespace backend

} // namespace HiCR
