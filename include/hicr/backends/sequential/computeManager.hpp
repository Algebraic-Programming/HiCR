/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file computeManager.hpp
 * @brief This file implements support for compute management of single processor systems
 * @author S. M. Martin
 * @date 10/12/2023
 */

#pragma once

#include "hwloc.h"
#include <hicr/backends/computeManager.hpp>
#include <hicr/backends/sequential/executionState.hpp>
#include <hicr/backends/sequential/executionUnit.hpp>
#include <hicr/backends/sequential/processingUnit.hpp>

namespace HiCR
{

namespace backend
{

namespace sequential
{

/**
 * Implementation of the HWloc-based HiCR Shared Memory backend's compute manager.
 *
 * It detects and returns the processing units detected by the HWLoc library
 */
class ComputeManager final : public backend::ComputeManager
{
  public:

  __USED__ inline ExecutionUnit *createExecutionUnit(ExecutionUnit::function_t executionUnit) override
  {
    return new ExecutionUnit(executionUnit);
  }

  /**
   * The constructor is employed to reserve memory required for hwloc
   */
  ComputeManager() : backend::ComputeManager() {}

  /**
   * The constructor is employed to free memory required for hwloc
   */
  ~ComputeManager() = default;

  private:

  /**
   * Sequential backend implementation that returns a single compute element.
   */
  __USED__ inline computeResourceList_t queryComputeResourcesImpl() override
  {
    // Only a single processing unit is created
    return computeResourceList_t({0});
  }

  __USED__ inline std::unique_ptr<HiCR::ProcessingUnit> createProcessingUnitImpl(computeResourceId_t resource) const override
  {
    return std::make_unique<ProcessingUnit>(resource);
  }
};

} // namespace sequential
} // namespace backend
} // namespace HiCR
