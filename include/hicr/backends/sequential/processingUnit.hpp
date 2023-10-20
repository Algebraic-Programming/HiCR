/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file processingUnit.hpp
 * @brief Implements the processing unit class for the sequential backend.
 * @author S. M. Martin
 * @date 11/9/2023
 */

#pragma once

#include <hicr/backends/sequential/executionState.hpp>
#include <hicr/backends/sequential/executionUnit.hpp>
#include <hicr/common/coroutine.hpp>
#include <hicr/common/exceptions.hpp>
#include <hicr/processingUnit.hpp>

namespace HiCR
{

namespace backend
{

namespace sequential
{

/**
 * Implementation of a procesing unit (a non-parallel process) for the sequential backend
 */
class ProcessingUnit final : public HiCR::ProcessingUnit
{
  public:

  /**
   * Constructor for the Process class
   *
   * \param process An id for the process (should be zero)
   */
  __USED__ inline ProcessingUnit(computeResourceId_t process) : HiCR::ProcessingUnit(process){};

  private:

  /**
   * Variable to hold the execution state to run
   */
  std::unique_ptr<HiCR::ExecutionState> _executionState;

  __USED__ inline void initializeImpl() override
  {
  }

  __USED__ inline void suspendImpl() override
  {
    // Yielding execution
    _executionState->suspend();
  }

  __USED__ inline void resumeImpl() override
  {
    // Resume coroutine
    _executionState->resume();
  }

  __USED__ inline void startImpl(std::unique_ptr<HiCR::ExecutionState> executionState) override
  {
    // Storing execution state internally
    _executionState = std::move(executionState);

    // And starting to run it
    _executionState->resume();
  }

  __USED__ inline void terminateImpl() override
  {
  }

  __USED__ inline void awaitImpl() override
  {
  }
};

} // end namespace sequential

} // namespace backend

} // namespace HiCR
