/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file processingUnit.hpp
 * @brief Provides a definition for a HiCR ProcessingUnit class
 * @author S. M. Martin
 * @date 13/7/2023
 */
#pragma once

#include <memory>
#include <hicr/definitions.hpp>
#include <hicr/L0/computeResource.hpp>
#include <hicr/L0/executionUnit.hpp>
#include <hicr/L0/executionState.hpp>

namespace HiCR
{

namespace L0
{

/**
 * This class represents an abstract definition for a Processing Unit resource in HiCR that:
 *
 * - Represents a single compute resource that has been instantiated for execution (as opposed of those who shall remain unused or unassigned).
 * - Is capable of executing or contributing to the execution of tasks.
 * - Is assigned, for example, to a worker to perform the work necessary to execute a task.
 * - This is a non-copy-able class
 */
class ProcessingUnit
{
  public:

  /**
   * Complete state set that a worker can be in
   */
  enum state_t
  {
    /**
     * The worker object has been instantiated but not initialized
     */
    uninitialized,

    /**
     * The worker has been ininitalized (or is back from executing) and can currently run
     */
    ready,

    /**
     * The worker has started executing
     */
    running,

    /**
     * The worker has started executing
     */
    suspended,

    /**
     * The worker has been issued for termination (but still running)
     */
    terminating,

    /**
     * The worker has terminated
     */
    terminated
  };

  /**
   * Disabled default constructor
   */
  ProcessingUnit() = delete;

  /**
   * Constructor for a processing unit
   *
   * \param computeResource The instance of the compute resource to instantiate, as indicated by the backend
   */
  __USED__ inline ProcessingUnit(std::shared_ptr<HiCR::L0::ComputeResource> computeResource) : _computeResource(computeResource){};

  virtual ~ProcessingUnit() = default;

  /**
   * Function to obtain the processing unit's state
   *
   * \return Retruns the current state
   */
  __USED__ inline ProcessingUnit::state_t getState() const
  {
    return _state;
  }

  /**
   * Initializes the resource and leaves it ready to execute work
   */
  __USED__ inline void initialize()
  {
    // Checking internal state
    if (_state != ProcessingUnit::uninitialized && _state != ProcessingUnit::terminated) HICR_THROW_RUNTIME("Attempting to initialize already initialized processing unit");

    // Calling PU-specific initialization
    initializeImpl();

    // Transitioning state
    _state = ProcessingUnit::ready;
  }

  /**
   * Starts running the resource and execute a previously initialized executionState object
   *
   * @param[in] executionState The execution state to start running with this processing  unit
   */
  __USED__ inline void start(std::unique_ptr<HiCR::L0::ExecutionState> executionState)
  {
    // Checking internal state
    if (_state != ProcessingUnit::ready) HICR_THROW_RUNTIME("Attempting to start processing unit that is not in the 'ready' state");

    // Transitioning state
    _state = ProcessingUnit::running;

    // Running internal implementation of the start function
    startImpl(std::move(executionState));
  }

  /**
   * Triggers the suspension of the resource. All the elements that make the resource remain active in memory, but will not execute.
   */
  __USED__ inline void suspend()
  {
    // Checking state
    if (_state != ProcessingUnit::running) HICR_THROW_RUNTIME("Attempting to suspend processing unit that is not in the 'running' state");

    // Transitioning state
    _state = ProcessingUnit::suspended;

    // Calling internal implementation of the suspend function
    suspendImpl();
  }

  /**
   * Resumes the execution of the resource.
   */
  __USED__ inline void resume()
  {
    // Checking state
    if (_state != ProcessingUnit::suspended) HICR_THROW_RUNTIME("Attempting to resume processing unit that is not in the 'suspended' state");

    // Transitioning state
    _state = ProcessingUnit::running;

    // Calling internal implementation of the resume function
    resumeImpl();
  }

  /**
   * Triggers the finalization the execution of the resource. This is an asynchronous operation, so returning from this function does not guarantee that the resource has terminated.
   */
  __USED__ inline void terminate()
  {
    // Checking state
    if (_state != ProcessingUnit::running) HICR_THROW_RUNTIME("Attempting to stop processing unit that is not in the 'running' state");

    // Transitioning state
    _state = ProcessingUnit::terminating;

    // Calling internal implementation of the terminate function
    terminateImpl();
  }

  /**
   * Suspends the execution of the caller until the finalization is ultimately completed
   */
  __USED__ inline void await()
  {
    // Checking state
    if (_state != ProcessingUnit::terminating && _state != ProcessingUnit::running && _state != ProcessingUnit::suspended)
      HICR_THROW_RUNTIME("Attempting to wait for a processing unit that has not yet started or has already terminated");

    // Calling internal implementation of the await function
    awaitImpl();

    // Transitioning state
    _state = ProcessingUnit::terminated;
  }

  /**
   * Returns the processing unit's associated compute resource
   *
   * \return The identifier of the compute resource associated to this processing unit.
   */
  __USED__ inline std::shared_ptr<ComputeResource> getComputeResource() { return _computeResource; }

  protected:

  /**
   * Internal implementation of the initialize routine
   */
  virtual void initializeImpl() = 0;

  /**
   * Internal implmentation of the start function
   *
   * @param[in] executionState The execution state to start running with this processing unit
   */
  virtual void startImpl(std::unique_ptr<ExecutionState> executionState) = 0;

  /**
   * Internal implementation of the suspend function
   */
  virtual void suspendImpl() = 0;

  /**
   * Internal implementation of the resume function
   */
  virtual void resumeImpl() = 0;

  /**
   * Internal implementation of the terminate function
   */
  virtual void terminateImpl() = 0;

  /**
   * Internal implementation of the await function
   */
  virtual void awaitImpl() = 0;

  private:

  /**
   * Represents the internal state of the processing unit. Uninitialized upon construction.
   */
  ProcessingUnit::state_t _state = ProcessingUnit::uninitialized;

  /**
   * Identifier of the compute resource associated to this processing unit
   */
  const std::shared_ptr<HiCR::L0::ComputeResource> _computeResource;
};

} // namespace L0

} // namespace HiCR
