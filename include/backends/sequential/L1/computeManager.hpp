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
#include <backends/sequential/L0/executionState.hpp>
#include <backends/sequential/L0/executionUnit.hpp>
#include <backends/sequential/L0/processingUnit.hpp>
#include <hicr/L0/computeResource.hpp>
#include <hicr/L1/computeManager.hpp>

namespace HiCR
{

namespace backend
{

namespace sequential
{

namespace L1
{

/**
 * Implementation of the HWloc-based HiCR Shared Memory backend's compute manager.
 *
 * It detects and returns the processing units detected by the HWLoc library
 */
class ComputeManager final : public HiCR::L1::ComputeManager
{
  public:

  __USED__ inline HiCR::L0::ExecutionUnit *createExecutionUnit(HiCR::L0::ExecutionUnit::function_t executionUnit) override
  {
    return new L0::ExecutionUnit(executionUnit);
  }

  /**
   * The constructor is employed to reserve memory required for hwloc
   */
  ComputeManager() : HiCR::L1::ComputeManager() {}

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
    auto hostCPU = new sequential::L0::ComputeResource();

    // Returning list
    return computeResourceList_t({hostCPU});
  }

  __USED__ inline std::unique_ptr<HiCR::L0::ProcessingUnit> createProcessingUnitImpl(HiCR::L0::ComputeResource* resource) const override
  {
    return std::make_unique<L0::ProcessingUnit>(resource);
  }
};

} // namespace L1

} // namespace sequential

} // namespace backend

} // namespace HiCR
