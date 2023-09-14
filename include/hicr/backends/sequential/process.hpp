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
  public:

  /**
   * Constructor for the Process class
   *
   * \param process An id for the process (should be zero)
   */
  __USED__ inline Process(computeResourceId_t process) : ProcessingUnit(process){};

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
