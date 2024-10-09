/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file worker.hpp
 * brief Provides a definition for the HiCR Worker class.
 * @author S. M. Martin
 * @date 7/7/2023
 */

#pragma once

#include <thread>
#include <memory>
#include <vector>
#include <set>
#include <unistd.h>
#include <hicr/core/definitions.hpp>
#include <hicr/core/exceptions.hpp>
#include <hicr/core/L0/processingUnit.hpp>
#include <hicr/backends/host/L1/computeManager.hpp>
#include "task.hpp"

namespace HiCR
{

namespace tasking
{

/**
 * Defines a standard type for a pull function.
 */
typedef std::function<HiCR::tasking::Task *()> pullFunction_t;

/**
 * Defines the worker class, which is in charge of executing tasks.
 *
 * To receive pending tasks for execution, the worker needs a pull function. Upon execution, the worker will constantly check the pull function in search for new tasks for execution.
 *
 * To execute a task, the worker needs to be assigned at least a computational resource capable to executing the type of task submitted.
 */
class Worker
{
  public:

  /**
   * Enumeration of possible task-related callbacks that can trigger a user-defined function callback
   */
  enum callback_t
  {
    /**
     * Triggered as the task starts or resumes execution
     */
    onWorkerStart,

    /**
     * Triggered as the worker is preempted into suspension by an asynchronous callback
     */
    onWorkerSuspend,

    /**
     * Triggered as the worker is resumed from suspension
     */
    onWorkerResume,

    /**
     * Triggered as the worker terminates
     */
    onWorkerTerminate,
  };

  /**
   * Type definition for the worker's callback map
   */
  typedef HiCR::tasking::CallbackMap<Worker *, callback_t> workerCallbackMap_t;

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
     * The worker is in the process of being suspended
     */
    suspending,

    /**
     * The worker has suspended
     */
    suspended,

    /**
     * The worker is in the process of being resumed
     */
    resuming,

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
   * Constructor for the worker class.
   *
   * \param[in] computeManager A backend's compute manager, meant to initialize and run the task's execution states.
   * \param[in] pullFunction A callback for the worker to get a new task to execute
   * @param[in] callbackMap Pointer to the callback map to be called by the worker
   */
  Worker(HiCR::L1::ComputeManager *computeManager, pullFunction_t pullFunction, workerCallbackMap_t *callbackMap = NULL)
    : _pullFunction(pullFunction),
      _computeManager(dynamic_cast<HiCR::backend::host::L1::ComputeManager *>(computeManager)),
      _callbackMap(callbackMap)
  {
    // Checking the passed compute manager is of a supported type
    if (_computeManager == NULL) HICR_THROW_LOGIC("HiCR workers can only be instantiated with a shared memory compute manager.");
  }

  virtual ~Worker() = default;

  /**
   * Queries the worker's internal state.
   *
   * @return The worker's internal state
   */
  __INLINE__ const state_t getState() { return _state; }

  /**
   * Sets the worker's callback map. This map will be queried whenever a state transition occurs, and if the map defines a callback for it, it will be executed.
   *
   * @param[in] callbackMap A pointer to an callback map
   */
  __INLINE__ void setCallbackMap(workerCallbackMap_t *callbackMap) { _callbackMap = callbackMap; }

  /**
   * Gets the task's callback map.
   *
   * @return A pointer to the worker's an callback map. nullptr, if not defined.
   */
  __INLINE__ workerCallbackMap_t *getCallbackMap() { return _callbackMap; }

  /**
   * Initializes the worker and its resources
   */
  __INLINE__ void initialize()
  {
    // Grabbing state value
    auto prevState = _state.load();

    // Checking we have at least one assigned resource
    if (_processingUnits.empty()) HICR_THROW_LOGIC("Attempting to initialize worker without any assigned resources");

    // Checking state
    if (prevState != state_t::uninitialized && prevState != state_t::terminated) HICR_THROW_RUNTIME("Attempting to initialize already initialized worker");

    // Initializing all resources
    for (auto &r : _processingUnits) r->initialize();

    // Transitioning state
    _state = state_t::ready;
  }

  /**
   * Initializes the worker's task execution loop
   */
  __INLINE__ void start()
  {
    // Grabbing state value
    auto prevState = _state.load();

    // Checking state
    if (prevState != state_t::ready) HICR_THROW_RUNTIME("Attempting to start worker that is not in the 'initialized' state");

    // Setting state
    _state = state_t::running;

    // Creating new execution unit (the processing unit must support an execution unit of 'host' type)
    auto executionUnit = HiCR::backend::host::L1::ComputeManager::createExecutionUnit([](void *worker) { ((HiCR::tasking::Worker *)worker)->mainLoop(); });

    // Creating worker's execution state
    auto executionState = _computeManager->createExecutionState(executionUnit, this);

    // Launching worker in the lead resource (first one to be added)
    _processingUnits[0]->start(std::move(executionState));
  }

  /**
   * Suspends the execution of the underlying resource(s). The resources are guaranteed to be suspended after this function is called
   * 
   * @return True, if the worker has been put to suspended; False, otherwise. The latter is returned if the thread wasn't running in the first place
   * 
   */
  __INLINE__ bool suspend()
  {
    // Doing an atomic exchange
    state_t expected  = state_t::running;
    bool    succeeded = _state.compare_exchange_weak(expected, state_t::suspending);

    // Checking exchange
    return succeeded;
  }

  /**
   * Resumes the execution of the underlying resource(s) after suspension
   * 
   * @return True, if the worker has been put to resume; False, otherwise. The latter is returned if the thread wasn't suspended in the first place
   */
  __INLINE__ bool resume()
  {
    // Doing an atomic exchange
    state_t expected  = state_t::suspended;
    bool    succeeded = _state.compare_exchange_weak(expected, state_t::resuming);

    // Checking exchange
    return succeeded;
  }

  /**
   * Terminates the worker's task execution loop. After stopping it can be restarted later
   */
  __INLINE__ void terminate()
  {
    // Transitioning state
    auto prevState = _state.exchange(state_t::terminating);

    // Checking state
    if (prevState != state_t::running && prevState != state_t::suspending) HICR_THROW_RUNTIME("Attempting to stop worker that is not in a terminate-able state");
  }

  /**
   * A function that will suspend the execution of the caller until the worker has stopped
   */
  __INLINE__ void await()
  {
    // Getting state
    auto prevState = _state.load();

    if (prevState != state_t::terminating && prevState != state_t::running && prevState != state_t::suspended && prevState != state_t::suspending && prevState != state_t::resuming)
      HICR_THROW_RUNTIME("Attempting to wait for a worker that has not yet started or has already terminated");

    // Wait for the resources to free up
    for (auto &p : _processingUnits) p->await();

    // Transitioning state
    _state = state_t::terminated;
  }

  /**
   * Adds a processing unit to the worker. The worker will freely use this resource during execution. The worker may contain multiple resources and resource types.
   *
   * @param[in] pu Processing unit to assign to the worker
   */
  __INLINE__ void addProcessingUnit(std::unique_ptr<HiCR::L0::ProcessingUnit> pu) { _processingUnits.push_back(std::move(pu)); }

  /**
   * Gets a reference to the workers assigned processing units.
   *
   * @return A container with the worker's resources
   */
  __INLINE__ std::vector<std::unique_ptr<HiCR::L0::ProcessingUnit>> &getProcessingUnits() { return _processingUnits; }

  /**
   * This function sets the sleep intervals for a worker after it has been suspended, and in between checks for resuming
   * 
   * @param[in] suspendIntervalMs The interval (milliseconds) a worker sleeps for before checking for suspension conditions again
   */
  __INLINE__ void setSuspendInterval(size_t suspendIntervalMs) { _suspendIntervalMs = suspendIntervalMs; }

  protected:

  /**
  *  This function runs in in intervals defined by _suspendIntervalMs to check whether the suspension conditions remain true
  *  
  *  @return True, if the worker must now resume; False, otherwise.
  */
  __INLINE__ virtual bool checkResumeConditions() { return _state == state_t::resuming; }

  private:

  /**
   * Function by which the worker can obtain new tasks to execute
   */
  const pullFunction_t _pullFunction;

  /**
   * The time to suspend a worker for, before checking suspend conditions again
   */
  size_t _suspendIntervalMs = 1000;

  /**
   * Represents the internal state of the worker. Uninitialized upon construction.
   */
  std::atomic<state_t> _state = state_t::uninitialized;

  /**
   * Group of resources the worker can freely use
   */
  std::vector<std::unique_ptr<HiCR::L0::ProcessingUnit>> _processingUnits;

  /**
   * Compute manager to use to instantiate and manage the worker's and task execution states
   */
  HiCR::backend::host::L1::ComputeManager *const _computeManager;

  /**
   *  Map of callbacks to trigger
   */
  workerCallbackMap_t *_callbackMap = NULL;

  /**
   * Internal loop of the worker in which it searchers constantly for tasks to run
   */
  __INLINE__ void mainLoop()
  {
    // Calling appropriate callback
    if (_callbackMap != NULL) _callbackMap->trigger(this, callback_t::onWorkerStart);

    // Start main worker loop (run until terminated)
    while (true)
    {
      // Attempt to get a task by executing the pull function
      auto task = _pullFunction();

      // If a task was returned, then start or execute it
      if (task != nullptr) [[likely]]
      {
        // If the task hasn't been initialized yet, we need to do it now
        if (task->getState() == HiCR::L0::ExecutionState::state_t::uninitialized)
        {
          // First, create new execution state for the processing unit
          auto executionState = _computeManager->createExecutionState(task->getExecutionUnit(), task);

          // Then initialize the task with the new execution state
          task->initialize(std::move(executionState));
        }

        // Now actually run the task
        task->run();
      }

      // Requesting processing units to terminate as soon as possible
      if (_state == state_t::suspending) [[unlikely]]
      {
        // Setting state as suspended
        _state = state_t::suspended;

        // Calling appropriate callback
        if (_callbackMap != NULL) _callbackMap->trigger(this, callback_t::onWorkerSuspend);

        // Suspending other processing units
        for (size_t i = 1; i < _processingUnits.size(); i++) _processingUnits[i]->suspend();

        // Putting current processing unit to check every so often
        while (checkResumeConditions() == false) usleep(_suspendIntervalMs * 1000);

        // Calling appropriate callback
        if (_callbackMap != NULL) _callbackMap->trigger(this, callback_t::onWorkerResume);

        // Resuming other processing units
        for (size_t i = 1; i < _processingUnits.size(); i++) _processingUnits[i]->resume();

        // Setting worker as running
        _state = state_t::running;
      }

      // Requesting processing units to terminate as soon as possible
      if (_state == state_t::terminating) [[unlikely]]
      {
        // Calling appropriate callback
        if (_callbackMap != NULL) _callbackMap->trigger(this, callback_t::onWorkerTerminate);

        // Terminate secondary processing units first
        for (size_t i = 1; i < _processingUnits.size(); i++) _processingUnits[i]->terminate();

        // Then terminate current processing unit
        _processingUnits[0]->terminate();

        // Return immediately
        return;
      }
    }
  }
}; // class Worker

} // namespace tasking

} // namespace HiCR