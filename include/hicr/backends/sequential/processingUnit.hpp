/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file thread.hpp
 * @brief Implements the processing unit class for the sequential backend.
 * @author S. M. Martin
 * @date 11/9/2023
 */

#pragma once

#include <hicr/common/coroutine.hpp>
#include <hicr/common/exceptions.hpp>
#include <hicr/processingUnit.hpp>
#include <hicr/backends/sequential/executionUnit.hpp>
#include <hicr/backends/sequential/executionState.hpp>

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

  __USED__ inline std::unique_ptr<HiCR::ExecutionState> createExecutionState(const HiCR::ExecutionUnit* executionUnit) override
  {
   // Getting up-casted pointer for the execution unit
   auto e = dynamic_cast<const sequential::ExecutionUnit*>(executionUnit);

   // Checking whether the execution unit passed is compatible with this backend
   if (e == NULL) HICR_THROW_FATAL("The passed execution of type '%s' is not supported by this backend\n", executionUnit->getType());

   // Creating and returning new execution state
   return std::make_unique<sequential::ExecutionState>(e);
  }

  private:

  /**
   * Coroutine to handle suspend/resume functionality
   */
  common::Coroutine *_coroutine;

  __USED__ inline void initializeImpl() override
  {
    // Creating new coroutine to run
    _coroutine = new common::Coroutine();

    return;
  }

  __USED__ inline void suspendImpl() override
  {
    // Yielding execution
    _coroutine->yield();
  }

  __USED__ inline void resumeImpl() override
  {
    // Resume coroutine
    _coroutine->resume();
  }

  __USED__ inline void startImpl(std::unique_ptr<HiCR::ExecutionState> executionState) override
  {
     // Calling function in the context of a suspendable coroutine
     _coroutine->start([&executionState](){ executionState->resume(); });

     _coroutine->resume();
  }


  __USED__ inline void terminateImpl() override
  {
    // Nothing to do for terminate
    return;
  }

  __USED__ inline void awaitImpl() override
  {
    // Deleting allocated coroutine
    delete _coroutine;

    return;
  }
};

} // end namespace sequential

} // namespace backend

} // namespace HiCR
