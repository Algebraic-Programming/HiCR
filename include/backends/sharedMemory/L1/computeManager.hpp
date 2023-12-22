/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file computeManager.hpp
 * @brief This is a minimal backend for compute management of (multi-threaded) shared memory systems
 * @authors S. M. Martin, O. Korakitis
 * @date 10/12/2023
 */

#pragma once

#include "hwloc.h"
#include <backends/sequential/L0/executionUnit.hpp>
#include <backends/sharedMemory/L0/computeResource.hpp>
#include <backends/sharedMemory/L0/processingUnit.hpp>
#include <hicr/L1/computeManager.hpp>
#include <memory>

namespace HiCR
{

namespace backend
{

namespace sharedMemory
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

  /**
   * Constructor for the compute manager class for the shared memory backend
   *
   * \param[in] topology An HWloc topology object that can be used to query the available computational resources
   */
  ComputeManager() : HiCR::L1::ComputeManager() {}

  /**
   * Default destructor
   */
  ~ComputeManager() = default;

  __USED__ inline HiCR::L0::ExecutionUnit *createExecutionUnit(sequential::L0::ExecutionUnit::function_t executionUnit) override
  {
    return new sequential::L0::ExecutionUnit(executionUnit);
  }

  __USED__ inline std::unique_ptr<HiCR::L0::ExecutionState> createExecutionState(HiCR::L0::ExecutionUnit *executionUnit) override
  {
    // Creating and returning new execution state
    return std::make_unique<sequential::L0::ExecutionState>(executionUnit);
  }
  
  private:

  __USED__ inline std::unique_ptr<HiCR::L0::ProcessingUnit> createProcessingUnitImpl(HiCR::L0::ComputeResource* computeResource) const override
  {
    return std::make_unique<L0::ProcessingUnit>(computeResource);
  }
};

} // namespace L1

} // namespace sharedMemory

} // namespace backend

} // namespace HiCR
