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
#include <hicr/core/L1/computeManager.hpp>
#include <hicr/backends/host/L0/executionUnit.hpp>
#include <hicr/backends/host/hwloc/L0/computeResource.hpp>
#include <hicr/backends/host/pthreads/L0/processingUnit.hpp>

namespace HiCR::backend::host::L1
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
  ComputeManager()
    : HiCR::L1::ComputeManager()
  {}

  /**
   * The constructor is employed to free memory required for hwloc
   */
  ~ComputeManager() override = default;

  /** This function enables the creation of an execution unit.
   *
   * Its default constructor takes a simple replicable CPU-executable function
   *
   * \param[in] executionUnit The replicable function to execute
   * @return The newly created execution unit
   */
  __INLINE__ static std::shared_ptr<HiCR::L0::ExecutionUnit> createExecutionUnit(const Coroutine::coroutineFc_t &executionUnit)
  {
    return std::make_shared<host::L0::ExecutionUnit>(executionUnit);
  }

  __INLINE__ std::unique_ptr<HiCR::L0::ExecutionState> createExecutionState(std::shared_ptr<HiCR::L0::ExecutionUnit> executionUnit, void *const argument = nullptr) const override
  {
    // Creating and returning new execution state
    return std::make_unique<host::L0::ExecutionState>(executionUnit, argument);
  }
};

} // namespace HiCR::backend::host::L1
