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

#include <hicr/definitions.hpp>
#include <hicr/exceptions.hpp>
#include <hicr/L0/executionUnit.hpp>

namespace HiCR
{

namespace L0
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
     * Internal state not yet allocated
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
   * This function starts a newly initialized execution states or resumes a suspended one
   */
  __INLINE__ void resume()
  {
    if (_state != state_t::initialized && _state != state_t::suspended)
      HICR_THROW_RUNTIME("Attempting to resume an execution state that is not in a initialized or suspended state (State: %d).\n", _state);

    // Setting state as running
    _state = state_t::running;

    // Calling internal implementation of the suspend routine
    resumeImpl();
  }

  /**
   * This function suspends the execution of a running execution state
   */
  __INLINE__ void suspend()
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
  __INLINE__ bool checkFinalization()
  {
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
  __INLINE__ state_t getState() { return _state; }

  /**
   * Default constructor is deleted to prevent instantiation without proper arguments
   */
  ExecutionState() = delete;

  /**
   * Default destructor
   */
  virtual ~ExecutionState() = default;

  protected:

  /**
   * To save memory, the initialization of execution states (i.e., allocation of required structures) is deferred until this function is called.
   *
   * \param[in] executionUnit Represents a replicable executable unit (e.g., function, kernel) to execute
   */
  ExecutionState(const std::shared_ptr<HiCR::L0::ExecutionUnit> executionUnit){};

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
  state_t _state = state_t::initialized;
};

} // namespace L0

} // namespace HiCR
