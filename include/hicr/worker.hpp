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

#include <hicr/backends/computeManager.hpp>
#include <hicr/common/definitions.hpp>
#include <hicr/common/exceptions.hpp>
#include <hicr/dispatcher.hpp>
#include <hicr/processingUnit.hpp>
#include <hicr/task.hpp>
#include <memory>
#include <unistd.h>
#include <pthread.h>

namespace HiCR
{

/**
 * Type definition for the set of dispatchers a worker is subscribed to
 */
typedef std::set<Dispatcher *> dispatcherSet_t;

class Worker;

/**
 * Static storage for remembering the executing worker
 */
parallelHashMap_t<pthread_t, Worker *> _currentWorkerMap;

/**
 * Function to return a pointer to the currently executing worker from a global context
 */
__USED__ static inline Worker *getCurrentWorker() { return _currentWorkerMap[pthread_self()]; }

/**
 * Defines the worker class, which is in charge of executing tasks.
 *
 * To receive pending tasks for execution, the worker needs to subscribe to task dispatchers. Upon execution, the worker will constantly check the dispatchers in search for new tasks for execution.
 *
 * To execute a task, the worker needs to be assigned at least a computational resource capable to executing the type of task submitted.
 */
class Worker
{
  public:

  /**
   * Constructor for the worker class.
   *
   * \param[in] computeManager A backend's compute manager, meant to initialize and run the task's execution states.
   */
  Worker(HiCR::backend::ComputeManager *computeManager) : _computeManager(computeManager) {}
  ~Worker() = default;

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
   * Queries the worker's internal state.
   *
   * @return The worker's internal state
   *
   * \internal This is not a thread safe operation.
   */
  __USED__ inline const state_t getState() { return _state; }

  /**
   * Initializes the worker and its resources
   */
  __USED__ inline void initialize()
  {
    // Checking we have at least one assigned resource
    if (_processingUnits.empty()) HICR_THROW_LOGIC("Attempting to initialize worker without any assigned resources");

    // Checking state
    if (_state != state_t::uninitialized && _state != state_t::terminated) HICR_THROW_RUNTIME("Attempting to initialize already initialized worker");

    // Initializing all resources
    for (auto &r : _processingUnits) r->initialize();

    // Transitioning state
    _state = state_t::ready;
  }

  /**
   * Initializes the worker's task execution loop
   */
  __USED__ inline void start()
  {
    // Checking state
    if (_state != state_t::ready) HICR_THROW_RUNTIME("Attempting to start worker that is not in the 'initialized' state");

    // Transitioning state
    _state = state_t::running;

    // Creating new execution unit (the processing unit must support an execution unit of 'sequential' type)
    auto executionUnit = _computeManager->createExecutionUnit([this]()
                                                              { this->mainLoop(); });

    // Creating worker's execution state
    auto executionState = _computeManager->createExecutionState();

    // Initializing execution state
    executionState->initialize(executionUnit);

    // Launching worker in the lead resource (first one to be added)
    _processingUnits[0]->start(std::move(executionState));

    // Free up memory
    delete executionUnit;
  }

  /**
   * Suspends the execution of the underlying resource(s). The resources are guaranteed to be suspended after this function is called
   */
  __USED__ inline void suspend()
  {
    // Checking state
    if (_state != state_t::running) HICR_THROW_RUNTIME("Attempting to suspend worker that is not in the 'running' state");

    // Transitioning state
    _state = state_t::suspended;

    // Suspending processing units
    for (auto &p : _processingUnits) p->suspend();
  }

  /**
   * Resumes the execution of the underlying resource(s) after suspension
   */
  __USED__ inline void resume()
  {
    // Checking state
    if (_state != state_t::suspended) HICR_THROW_RUNTIME("Attempting to resume worker that is not in the 'suspended' state");

    // Transitioning state
    _state = state_t::running;

    // Suspending resources
    for (auto &p : _processingUnits) p->resume();
  }

  /**
   * Terminates the worker's task execution loop. After stopping it can be restarted later
   */
  __USED__ inline void terminate()
  {
    // Checking state
    if (_state != state_t::running) HICR_THROW_RUNTIME("Attempting to stop worker that is not in the 'running' state");

    // Transitioning state
    _state = state_t::terminating;
  }

  /**
   * A function that will suspend the execution of the caller until the worker has stopped
   */
  __USED__ inline void await()
  {
    if (_state != state_t::terminating && _state != state_t::running && _state != state_t::suspended)
      HICR_THROW_RUNTIME("Attempting to wait for a worker that has not yet started or has already terminated");

    // Wait for the resources to free up
    for (auto &p : _processingUnits) p->await();

    // Transitioning state
    _state = state_t::terminated;
  }

  /**
   * Subscribes the worker to a task dispatcher. During execution, the worker will constantly query the dispatcher for new tasks to execute.
   *
   * @param[in] dispatcher The dispatcher to subscribe the worker to
   */
  __USED__ inline void subscribe(Dispatcher *dispatcher) { _dispatchers.insert(dispatcher); }

  /**
   * Adds a processing unit to the worker. The worker will freely use this resource during execution. The worker may contain multiple resources and resource types.
   *
   * @param[in] pu Processing unit to assign to the worker
   */
  __USED__ inline void addProcessingUnit(ProcessingUnit *pu) { _processingUnits.push_back(pu); }

  /**
   * Gets a reference to the workers assigned processing units.
   *
   * @return A container with the worker's resources
   */
  __USED__ inline processingUnitList_t &getProcessingUnits() { return _processingUnits; }

  /**
   * Gets a reference to the dispatchers the worker has been subscribed to
   *
   * @return A container with the worker's subscribed dispatchers
   */
  __USED__ inline dispatcherSet_t &getDispatchers() { return _dispatchers; }

  private:

  /**
   * Represents the internal state of the worker. Uninitialized upon construction.
   */
  state_t _state = state_t::uninitialized;

  /**
   * Dispatchers that this resource is subscribed to
   */
  dispatcherSet_t _dispatchers;

  /**
   * Group of resources the worker can freely use
   */
  processingUnitList_t _processingUnits;

  /**
   * Compute manager to use to instantiate and manage the worker's and task execution states
   */
  HiCR::backend::ComputeManager *const _computeManager;

  /**
   * Internal loop of the worker in which it searchers constantly for tasks to run
   */
  __USED__ inline void mainLoop()
  {
    // Setting the pointer to the current worker into the thread local storage
    _currentWorkerMap[pthread_self()] = this;

    while (_state == state_t::running)
    {
      for (auto dispatcher : _dispatchers)
      {
        // Attempt to both pop and pull from dispatcher
        auto task = dispatcher->pull();

        // If a task was returned, then start or execute it
        if (task != NULL) [[likely]]
        {
          // If the task hasn't been initialized yet, we need to do it now
          if (task->getState() == ExecutionState::state_t::uninitialized)
          {
            // First, create new execution state for the processing unit
            auto executionState = _computeManager->createExecutionState();

            // Then initialize the task with the new execution state
            task->initialize(std::move(executionState));
          }

          // Now actually run the task
          task->run();
        }

        // If worker has been suspended, handle it now
        if (_state == state_t::suspended) [[unlikely]]
        {
          // Suspend secondary processing units first
          for (size_t i = 1; i < _processingUnits.size(); i++) _processingUnits[i]->suspend();

          // Then suspend current processing unit
          _processingUnits[0]->suspend();
        }

        // Requesting processing units to terminate as soon as possible
        if (_state == state_t::terminating) [[unlikely]]
        {
          // Terminate secondary processing units first
          for (size_t i = 1; i < _processingUnits.size(); i++) _processingUnits[i]->terminate();

          // Then terminate current processing unit
          _processingUnits[0]->terminate();

          // Return immediately
          return;
        }
      }
    }
  }
};

} // namespace HiCR
