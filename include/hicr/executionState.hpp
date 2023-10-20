/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file executeState.hpp
 * @brief Provides a base definition for a HiCR ExecutionState class
 * @author S. M. Martin
 * @date 13/10/2023
 */
#pragma once

#include <hicr/executionUnit.hpp>

namespace HiCR
{

class ExecutionState
{
  public:

  /**
   * Complete state set that a task can be in
   */
  enum state_t
  {
    /**
     * Internal state not yet allocated -- set automatically upon creation
     */
    uninitialized,

    /**
     * Ready to run (internal state created)
     */
    initialized,

    /**
     * Indicates that the task is currently running
     */
    running,

    /**
     * Set by the task if it suspends for an asynchronous operation
     */
    suspended,

    /**
     * Set by the task upon complete termination
     */
    finished
  };

  __USED__ inline void initialize(const HiCR::ExecutionUnit *executionUnit)
  {
    if (_state != state_t::uninitialized) HICR_THROW_LOGIC("Attempting to initialize an execution state that has already been initialized (State: %d).\n", _state);

    // Calling internal implementation of the initialization routine
    initializeImpl(executionUnit);

    // Setting state as initialized
    _state = state_t::initialized;
  }

  __USED__ inline void resume()
  {
    if (_state != state_t::initialized && _state != state_t::suspended) HICR_THROW_RUNTIME("Attempting to resume an execution state that is not in a initialized or suspended state (State: %d).\n", _state);

    // Setting state as running
    _state = state_t::running;

    // Calling internal implementation of the suspend routine
    resumeImpl();
  }

  __USED__ inline void suspend()
  {
    if (_state != state_t::running) HICR_THROW_RUNTIME("Attempting to suspend an execution state that is not in a running state (State: %d).\n", _state);

    // Setting state as suspended
    _state = state_t::suspended;

    // Calling internal implementation of the suspend routine
    suspendImpl();
  }

  __USED__ inline bool checkFinalization()
  {
    if (_state == state_t::uninitialized) HICR_THROW_LOGIC("Attempting to check for finalization in an execution state that has not been initialized (State: %d).\n", _state);

    // Calling internal implementation of the checkFinalization routine
    auto isFinished = checkFinalizationImpl();

    // Updating internal state
    if (isFinished == true) _state = state_t::finished;

    // Returning value
    return isFinished;
  }

  __USED__ inline state_t getState() { return _state; }

  ExecutionState() = default;
  virtual ~ExecutionState() = default;

  protected:

  virtual void initializeImpl(const HiCR::ExecutionUnit *executionUnit) = 0;
  virtual void resumeImpl() = 0;
  virtual void suspendImpl() = 0;
  virtual bool checkFinalizationImpl() = 0;

  private:

  state_t _state = state_t::uninitialized;
};

} // namespace HiCR
