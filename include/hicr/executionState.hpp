/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file executionState.hpp
 * @brief Provides a base definition for a HiCR ExecutionState class
 * @author S. M. Martin
 * @date 13/10/2023
 */
#pragma once

#include <hicr/executionUnit.hpp>

namespace HiCR
{

/**
 * This class is an abstract representation of the lifetime of an execution unit.
 * It exposes initialization, suspension and resume functionalities that should (ideally) be implemented for all execution/processing unit combinations.
 */
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

  /**
   * To save memory, the initialization of execution states (i.e., allocation of required structures) is deferred until this function is called.
   *
   * \param[in] executionUnit Represents a replicable executable unit (e.g., function, kernel) to execute
   */
  __USED__ inline void initialize(const HiCR::ExecutionUnit *executionUnit)
  {
    if (_state != state_t::uninitialized) HICR_THROW_LOGIC("Attempting to initialize an execution state that has already been initialized (State: %d).\n", _state);

    // Calling internal implementation of the initialization routine
    initializeImpl(executionUnit);

    // Setting state as initialized
    _state = state_t::initialized;
  }

  /**
   * This function starts a newly initialized execution states or resumes a suspended one
   */
  __USED__ inline void resume()
  {
    if (_state != state_t::initialized && _state != state_t::suspended) HICR_THROW_RUNTIME("Attempting to resume an execution state that is not in a initialized or suspended state (State: %d).\n", _state);

    // Setting state as running
    _state = state_t::running;

    // Calling internal implementation of the suspend routine
    resumeImpl();
  }

  /**
   * This function suspends the execution of a running execution state
   */
  __USED__ inline void suspend()
  {
    if (_state != state_t::running) HICR_THROW_RUNTIME("Attempting to suspend an execution state that is not in a running state (State: %d).\n", _state);

    // Setting state as suspended
    _state = state_t::suspended;

    // Calling internal implementation of the suspend routine
    suspendImpl();
  }

  /**
   * This function actively checks for the finalization of an initialized execution state
   *
   * \return True, if the execution has finalized; False, otherwise.
   */
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

  /**
   * This function returns the current state of the execution
   *
   * \return The current execution state
   */
  __USED__ inline state_t getState() { return _state; }

  ExecutionState() = default;
  virtual ~ExecutionState() = default;

  protected:

  /**
   * Backend-specific implementation of the initialize function
   *
   * \param[in] executionUnit Represents a replicable executable unit (e.g., function, kernel) to execute
   */
  virtual void initializeImpl(const HiCR::ExecutionUnit *executionUnit) = 0;

  /**
   * Backend-specific implementation of the resume function
   */
  virtual void resumeImpl() = 0;

  /**
   * Backend-specific implementation of the suspend function
   */
  virtual void suspendImpl() = 0;

  /**
   * Backend-specific implementation of the checkFinalization function
   *
   * \return True, if the execution has finalized; False, otherwise.
   */
  virtual bool checkFinalizationImpl() = 0;

  private:

  /**
   * Storage for the internal execution state
   */
  state_t _state = state_t::uninitialized;
};

} // namespace HiCR
