/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file computeManager.hpp
 * @brief This file contains the abstract definition of the compute manager for host (CPU) backends
 * @authors S. M. Martin, O. Korakitis
 * @date 10/12/2023
 */

#pragma once

#include <memory>
#include <hicr/L1/computeManager.hpp>
#include <backends/sharedMemory/L0/executionUnit.hpp>
#include <backends/sharedMemory/hwloc/L0/computeResource.hpp>
#include <backends/sharedMemory/pthreads/L0/processingUnit.hpp>


namespace HiCR
{

namespace backend
{

namespace sharedMemory
{

namespace L1
{

/**
 * Abstract common implementation of HiCR host (CPU) backend's compute managers.
 */
class ComputeManager : public HiCR::L1::ComputeManager
{
  public:
  
  /**
   * The constructor is employed to reserve memory required for hwloc
   */
  ComputeManager() : HiCR::L1::ComputeManager() {}

  /**
   * The constructor is employed to free memory required for hwloc
   */
  ~ComputeManager() = default;

  /** This function enables the creation of an execution unit.
   *
   * Its default constructor takes a simple replicable CPU-executable function
   *
   * \param[in] executionUnit The replicable function to execute
   * @return The newly created execution unit
   */
  __USED__ inline std::shared_ptr<HiCR::L0::ExecutionUnit> createExecutionUnit(Coroutine::coroutineFc_t executionUnit) const
  {
    return std::make_shared<sharedMemory::L0::ExecutionUnit>(executionUnit);
  }

  __USED__ inline std::unique_ptr<HiCR::L0::ExecutionState> createExecutionState(std::shared_ptr<HiCR::L0::ExecutionUnit> executionUnit) const override
  {
    // Creating and returning new execution state
    return std::make_unique<sharedMemory::L0::ExecutionState>(executionUnit);
  }

  virtual std::unique_ptr<HiCR::L0::ProcessingUnit> createProcessingUnit(std::shared_ptr<HiCR::L0::ComputeResource> computeResource) const = 0;
};

} // namespace L1

} // namespace sharedMemory

} // namespace backend

} // namespace HiCR
