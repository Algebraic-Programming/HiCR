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

#include <backends/sequential/L0/executionState.hpp>
#include <backends/sequential/L0/executionUnit.hpp>
#include <backends/sequential/L0/computeResource.hpp>
#include <hicr/L0/computeResource.hpp>
#include <hicr/L0/executionUnit.hpp>
#include <hicr/L0/processingUnit.hpp>
#include <hicr/common/coroutine.hpp>
#include <hicr/common/exceptions.hpp>

namespace HiCR
{

namespace backend
{

namespace sequential
{

namespace L0
{

/**
 * Implementation of a procesing unit (a non-parallel process) for the sequential backend
 */
class ProcessingUnit final : public HiCR::L0::ProcessingUnit
{
  public:

  /**
   * Constructor for the Process class
   *
   * \param processor An id for the process (should be zero)
   */
  __USED__ inline ProcessingUnit(HiCR::L0::ComputeResource* computeResource) : HiCR::L0::ProcessingUnit(computeResource)
  {
    // Getting up-casted pointer for the MPI instance
    auto c = dynamic_cast<L0::ComputeResource *>(computeResource);

    // Checking whether the execution unit passed is compatible with this backend
    if (c == NULL) HICR_THROW_LOGIC("The passed compute resource is not supported by this processing unit type\n");
  };

  private:

  /**
   * Variable to hold the execution state to run
   */
  std::unique_ptr<HiCR::L0::ExecutionState> _executionState;

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

  __USED__ inline void startImpl(std::unique_ptr<HiCR::L0::ExecutionState> executionState) override
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

} // namespace L0

} // namespace sequential

} // namespace backend

} // namespace HiCR
