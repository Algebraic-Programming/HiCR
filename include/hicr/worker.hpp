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

#include <hicr/computeResource.hpp>
#include <hicr/dispatcher.hpp>
#include <hicr/task.hpp>

namespace HiCR
{

namespace worker
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
 * Defines the worker class, which is in charge of executing tasks.
 *
 * To receive pending tasks for execution, the worker needs to subscribe to task dispatchers. Upon execution, the worker will constantly check the dispatchers in search for new tasks for execution.
 *
 * To execute a task, the worker needs to be assigned at least a computational resource capable to executing the type of task submitted.
 */
class Worker
{
  private:
  /**
   * Represents the internal state of the worker. Uninitialized upon construction.
   */
  worker::state_t _state = worker::uninitialized;

  /**
   * Dispatchers that this resource is subscribed to
   */
  std::set<Dispatcher *> _dispatchers;

  /**
   * Group of resources the worker can freely use
   */
  std::vector<ComputeResource *> _computeResources;

  /**
   * Internal loop of the worker in which it searchers constantly for tasks to run
   */
  void mainLoop()
  {
    while (_state == worker::running)
    {
      for (auto dispatcher : _dispatchers)
      {
        // Attempt to both pop and pull from dispatcher
        auto task = dispatcher->pull(this);

        // If a task was returned, then execute it
        if (task != NULL) task->run(this);

        // If worker has been suspended, handle it now
        if (_state == worker::suspended) _computeResources[0]->suspend();
      }
    }
  }

  public:
  Worker() = default;
  ~Worker() = default;

  /**
   * Initializes the worker and its resources
   */
  void initialize()
  {
    // Checking state
    if (_state != worker::uninitialized) LOG_ERROR("Attempting to initialize already initialized worker");

    // Checking we have at least one assigned resource
    if (_computeResources.empty()) LOG_ERROR("Attempting to initialize worker without any assigned resources");

    // Initializing all resources
    for (auto r : _computeResources) r->initialize();

    // Transitioning state
    _state = worker::ready;
  }

  /**
   * Initializes the worker's task execution loop
   */
  void start()
  {
    // Checking state
    if (_state != worker::ready) LOG_ERROR("Attempting to start worker that is not in the 'initialized' state");

    // Checking we have at least one assigned resource
    if (_computeResources.empty()) LOG_ERROR("Attempting to start worker without any assigned resources");

    // Transitioning state
    _state = worker::running;

    // Launching worker in the lead resource (first one to be added)
    _computeResources[0]->run([this]()
                              { this->mainLoop(); });
  }

  /**
   * Suspends the execution of the underlying resource(s). The resources are guaranteed to be suspended after this function is called
   */
  void suspend()
  {
    // Checking state
    if (_state != worker::running) LOG_ERROR("Attempting to suspend worker that is not in the 'running' state");

    // Transitioning state
    _state = worker::suspended;
  }

  /**
   * Resumes the execution of the underlying resource(s) after suspension
   */
  void resume()
  {
    // Checking state
    if (_state != worker::suspended) LOG_ERROR("Attempting to resume worker that is not in the 'suspended' state");

    // Transitioning state
    _state = worker::running;

    // Suspending resources
    for (auto &r : _computeResources) r->resume();
  }

  /**
   * Terminates the worker's task execution loop. After stopping it can be restarted later
   */
  void terminate()
  {
    // Checking state
    if (_state != worker::running) LOG_ERROR("Attempting to stop worker that is not in the 'running' state");

    // Transitioning state
    _state = worker::terminating;
  }

  /**
   * A function that will suspend the execution of the caller until the worker has stopped
   */
  void await()
  {
    // Checking state
    if (_state != worker::terminating && _state != worker::running && _state != worker::suspended) LOG_ERROR("Attempting to wait for a worker that is not in the 'terminated', 'suspended' or 'running' state");

    // Wait for the resource to free up
    _computeResources[0]->await();

    // Transitioning state
    _state = worker::terminated;
  }

  /**
   * Subscribes the worker to a task dispatcher. During execution, the worker will constantly query the dispatcher for new tasks to execute.
   *
   * \param[in] dispatcher The dispatcher to subscribe the worker to
   */
  void subscribe(Dispatcher *dispatcher) { _dispatchers.insert(dispatcher); }

  /**
   * Adds a computational resource to the worker. The worker will freely use this resource during execution. The worker may contain multiple resources and resource types.
   *
   * \param[in] resource Resource to add to the worker
   */
  void addResource(ComputeResource *resource) { _computeResources.push_back(resource); }

  /**
   * Gets a reference to the workers assigned resources.
   *
   * \return A container with the worker's resources
   */
  std::vector<ComputeResource *> &getResources() { return _computeResources; }

  /**
   * Gets a reference to the dispatchers the worker has been subscribed to
   *
   * \return A container with the worker's subscribed dispatchers
   */
  std::set<Dispatcher *> &getDispatchers() { return _dispatchers; }
};

} // namespace HiCR
