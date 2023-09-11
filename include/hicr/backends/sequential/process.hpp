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

namespace HiCR
{

namespace backend
{

namespace sequential
{

/**
 * Implementation of a kernel-level thread as processing unit for the shared memory backend.
 *
 * This implementation uses PThreads as backend for the creation and management of OS threads..
 */
class Process final : public ProcessingUnit
{
  private:

 /**
  * Coroutine to handle suspend/resume functionality
  */
  Coroutine _coroutine;

  public:

  /**
   * Constructor for the Process class
   *
   * \param core Represents the core affinity to associate this processing unit to
   */
  __USED__ inline Process(computeResourceId_t process) : ProcessingUnit(process){};

  __USED__ inline void initialize() override
  {
   // Nothing to do for initialize
  return;
  }

  __USED__ inline void suspend() override
  {
   // Yielding execution
   _coroutine.yield();
  }

  __USED__ inline void resume() override
  {
   // Resume coroutine
  _coroutine.resume();
  }

  __USED__ inline void run(processingUnitFc_t fc) override
  {
    // Calling function in the context of a suspendable coroutine
   _coroutine.start([fc](void* arg){ fc(); }, NULL);
  }

  __USED__ inline void finalize() override
  {
   // Nothing to do for finalize
  return;
  }

  __USED__ inline void await() override
  {
    // Nothing to do for await
   return;
  }
};

} // end namespace sequential

} // namespace backend

} // namespace HiCR
