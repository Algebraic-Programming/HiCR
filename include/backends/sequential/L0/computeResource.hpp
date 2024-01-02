/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file computeResource.hpp
 * @brief This file implements the compute resource class for the sequential backend
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

namespace sequential
{

namespace L0
{

/**
 * This class represents a compute resource, as visible by the sequential backend. That is, a sequential process(or) with no information about locality and no assumption of multi-processing support.
 */
class ComputeResource final : public HiCR::L0::ComputeResource
{
  public:

  /**
   * Constructor for the compute resource class of the sequential backend
   */
  ComputeResource() : HiCR::L0::ComputeResource(){};

  /**
   * Default destructor
   */
  ~ComputeResource() = default;

  __USED__ inline std::string getType() const override { return "Sequential Process"; }
};

} // namespace L0

} // namespace sequential

} // namespace backend

} // namespace HiCR
