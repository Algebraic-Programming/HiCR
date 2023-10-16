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
  __USED__ inline ProcessingUnit(computeResourceId_t process) : ProcessingUnit(process){};

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

  __USED__ inline void startImpl(processingUnitFc_t fc) override
  {
    // Calling function in the context of a suspendable coroutine
    _coroutine->start([fc](void *arg)
                      { fc(); },
                      NULL);
  }

  __USED__ inline void startImpl(ExecutionUnit* executionUnit)
  {
   // The passed compute unit must be of type sequential Function
   auto functionPtr = dynamic_cast<Function*>(executionUnit);

   // If the cast wasn't successful, then this fails the execution
   if (functionPtr == NULL)  HICR_THROW_LOGIC("The provided compute unit of type (%s) is not supported by this backend", executionUnit->identifyExecutionUnitType());

   // Run function
   startImpl([functionPtr]() { functionPtr->getFunction()(NULL); });
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
