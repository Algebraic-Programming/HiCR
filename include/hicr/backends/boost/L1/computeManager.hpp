/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file computeManager.hpp
 * @brief This file implements the pthread-based compute manager for host (CPU) backends
 * @authors S. M. Martin, O. Korakitis
 * @date 10/12/2023
 */

#pragma once

#include <memory>
#include <hicr/core/exceptions.hpp>
#include <hicr/backends/boost/L0/executionState.hpp>
#include <hicr/core/L1/computeManager.hpp>

namespace HiCR::backend::boost::L1
{

/**
 * Defines the function type including a closure pointer to be accepted for executiong by a pthread processing unit
 */
using pthreadFc_t = std::function<void(void *)>;

/**
 * Implementation of the pthread-based HiCR Shared Memory backend's compute manager.
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
   * \param[in] coroutineFunction The replicable function to execute
   * @return The newly created execution unit
   */
  __INLINE__ static std::shared_ptr<HiCR::L0::ExecutionUnit> createExecutionUnit(const Coroutine::coroutineFc_t &coroutineFunction)
  {
    return std::make_shared<boost::L0::ExecutionUnit>(coroutineFunction);
  }

  __INLINE__ std::unique_ptr<HiCR::L0::ExecutionState> createExecutionState(std::shared_ptr<HiCR::L0::ExecutionUnit> executionUnit, void *const argument = nullptr) const override
  {
    // Creating and returning new execution state
    return std::make_unique<boost::L0::ExecutionState>(executionUnit, argument);
  }

  [[nodiscard]] __INLINE__ std::unique_ptr<HiCR::L0::ProcessingUnit> createProcessingUnit(std::shared_ptr<HiCR::L0::ComputeResource> computeResource) const override
  {
    {
      HICR_THROW_LOGIC("This backend does not implement this function");
    }
  }

  private:

  __INLINE__ void initializeImpl(std::unique_ptr<HiCR::L0::ProcessingUnit> &processingUnit) override
  {
    {
      HICR_THROW_LOGIC("This backend does not implement this function");
    }
  }

  __INLINE__ void startImpl(std::unique_ptr<HiCR::L0::ProcessingUnit> &processingUnit, std::unique_ptr<HiCR::L0::ExecutionState> &executionState) override
  {
    {
      HICR_THROW_LOGIC("This backend does not implement this function");
    }
  }

  __INLINE__ void suspendImpl(std::unique_ptr<HiCR::L0::ProcessingUnit> &processingUnit) override
  {
    {
      HICR_THROW_LOGIC("This backend does not implement this function");
    }
  }

  __INLINE__ void resumeImpl(std::unique_ptr<HiCR::L0::ProcessingUnit> &processingUnit) override
  {
    {
      HICR_THROW_LOGIC("This backend does not implement this function");
    }
  }

  __INLINE__ void terminateImpl(std::unique_ptr<HiCR::L0::ProcessingUnit> &processingUnit) override
  {
    {
      HICR_THROW_LOGIC("This backend does not implement this function");
    }
  }

  __INLINE__ void awaitImpl(std::unique_ptr<HiCR::L0::ProcessingUnit> &processingUnit) override
  {
    {
      HICR_THROW_LOGIC("This backend does not implement this function");
    }
  }
};

} // namespace HiCR::backend::boost::L1
