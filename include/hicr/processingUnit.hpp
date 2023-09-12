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
#include <set>

#include <hicr/common/definitions.hpp>
#include <hicr/dispatcher.hpp>

namespace HiCR
{

/**
 * Type definition for a generic memory space identifier
 */
typedef uint64_t computeResourceId_t;

/**
 * Definition for function to run at resource
 */
typedef std::function<void(void)> processingUnitFc_t;

namespace processingUnit
{

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

} // namespace worker


/**
 * This class represents an abstract definition for a Processing Unit resource in HiCR that:
 *
 * - Represents a single computational resource that has been instantiated for execution (as opposed of those who shall remain unused or unassigned).
 * - Is capable of executing or contributing to the execution of tasks.
 * - Is assigned to a worker to perform the work necessary to execute a task.
 */
class ProcessingUnit
{
  private:

  /**
   * Represents the internal state of the processing unit. Uninitialized upon construction.
   */
  processingUnit::state_t _state = processingUnit::uninitialized;

  /**
   * Identifier of the compute resource associated to this processing unit
   */
  computeResourceId_t _computeResourceId;

  protected:

  /**
   * Internal implementation of the initialize routine
   */
  virtual void initializeImpl() = 0;

  /**
   * Internal implmentation of the start function
   *
   * @param[in] fc The function to execute by the resource
   */
  virtual void startImpl(processingUnitFc_t fc) = 0;

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

  public:

  /**
   * Disabled default constructor
   */
  ProcessingUnit() = delete;

  /**
   * A processing unit is created to instantiate a single compute resource
   *
   * \param computeResourceId The identifier of the compute resource to instantiate, as indicated by the backend
   */
  __USED__ inline ProcessingUnit(computeResourceId_t computeResourceId) : _computeResourceId(computeResourceId){};

  virtual ~ProcessingUnit() = default;

  /**
   * Initializes the resource and leaves it ready to execute work
   */
  __USED__ inline void initialize()
  {
   // Checking internal state
   if (_state != processingUnit::uninitialized && _state != processingUnit::terminated) HICR_THROW_RUNTIME("Attempting to initialize already initialized processing unit");

   // Calling PU-specific initialization
   initializeImpl();

   // Transitioning state
   _state = processingUnit::ready;
  }

  /**
   * Starts running the resource and execute a user-defined function
   *
   * @param[in] fc The function to execute by the resource
   */
  __USED__ inline void start(processingUnitFc_t fc)
  {
   // Checking internal state
   if (_state != processingUnit::ready) HICR_THROW_RUNTIME("Attempting to start processing unit that is not in the 'initialized' state");

   // Transitioning state
   _state = processingUnit::running;

   // Running internal implementation of the start function
   startImpl(fc);
  }

  /**
   * Triggers the suspension of the resource. All the elements that make the resource remain active in memory, but will not execute.
   */
  __USED__ inline void suspend()
  {
   // Checking state
   if (_state != processingUnit::running) HICR_THROW_RUNTIME("Attempting to suspend processing unit that is not in the 'running' state");

   // Transitioning state
   _state = processingUnit::suspended;

   // Calling internal implementation of the suspend function
   suspendImpl();
  }

  /**
   * Resumes the execution of the resource.
   */
  __USED__ inline void resume()
  {
   // Checking state
   if (_state != processingUnit::suspended) HICR_THROW_RUNTIME("Attempting to resume processing unit that is not in the 'suspended' state");

   // Transitioning state
   _state = processingUnit::running;

   // Calling internal implementation of the resume function
   resumeImpl();
  }

  /**
   * Triggers the finalization the execution of the resource. This is an asynchronous operation, so returning from this function does not guarantee that the resource has terminated.
   */
  __USED__ inline void terminate()
  {
   // Checking state
   if (_state != processingUnit::running) HICR_THROW_RUNTIME("Attempting to stop processing unit that is not in the 'running' state");

   // Transitioning state
   _state = processingUnit::terminating;

   // Calling internal implementation of the terminate function
   terminateImpl();
  }

  /**
   * Suspends the execution of the caller until the finalization is ultimately completed
   */
  __USED__ inline void await()
  {
   // Checking state
   if (_state != processingUnit::terminating && _state != processingUnit::running && _state != processingUnit::suspended)
     HICR_THROW_RUNTIME("Attempting to wait for a processing unit that is not in the 'terminated', 'suspended' or 'running' state");

   // Calling internal implementation of the await function
   awaitImpl();

   // Transitioning state
   _state = processingUnit::terminated;
  }

  /**
   * Returns the processing unit's associated compute resource
   *
   * \return The identifier of the compute resource associated to this processing unit.
   */
  __USED__ inline computeResourceId_t getComputeResourceId() { return _computeResourceId; }

};

} // namespace HiCR
