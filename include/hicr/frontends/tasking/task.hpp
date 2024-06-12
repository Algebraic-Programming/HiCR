/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file deployer.hpp
 * @brief This file implements the HiCR task class
 * @author Sergio Martin
 * @date 8/8/2023
 */

#pragma once

#include <memory>
#include <queue>
#include <vector>
#include <mutex>
#include <hicr/core/definitions.hpp>
#include <hicr/core/exceptions.hpp>
#include <hicr/core/L0/executionState.hpp>
#include <hicr/core/L0/executionUnit.hpp>
#include <hicr/core/L0/processingUnit.hpp>
#include "eventMap.hpp"
#include "common.hpp"

namespace HiCR
{

namespace tasking
{

/**
 * Key identifier for thread-local identification of currently running task
 */
extern pthread_key_t _taskPointerKey;

/**
 * This class defines the basic execution unit managed by TaskR.
 *
 * It includes a function to execute, an internal state, and an event map that triggers callbacks (if defined) whenever a state transition occurs.
 *
 * The function represents the entire lifetime of the task. That is, a task executes a single function, the one provided by the user, and will reach a terminated state after the function is fully executed.
 *
 * A task may be suspended before the function is fully executed. This is either by voluntary yielding, or by reaching an synchronous operation that prompts it to suspend. These two suspension reasons will result in different states.
 */
class Task
{
  public:

  /**
   * Task label type
   */
  typedef uint64_t label_t;

  /**
   * Enumeration of possible task-related events that can trigger a user-defined function callback
   */
  enum event_t
  {
    /**
     * Triggered as the task starts or resumes execution
     */
    onTaskExecute,

    /**
     * Triggered as the task is preempted into suspension by an asynchronous event
     */
    onTaskSuspend,

    /**
     * Triggered as the task finishes execution
     */
    onTaskFinish,

    /**
     * Triggered as the task receives a sync signal (used for mutual exclusion mechanisms)
    */
    onTaskSync,
  };

  /**
   * Type definition for the task's event map
   */
  typedef HiCR::tasking::EventMap<Task, event_t> taskEventMap_t;

  Task()  = delete;
  ~Task() = default;

  /**
   * Constructor for the TaskR task class. It requires a user-defined function to execute and a label.
   * The task is considered finished when the function runs to completion.
   *
   * @param[in] label A user-defined unique identifier for the task. It is required for dependency management
   * @param[in] executionUnit Specifies the function/kernel to execute.
   * @param[in] eventMap Pointer to the event map callbacks to be called by the task
   */
  __INLINE__ Task(const label_t label, std::shared_ptr<HiCR::L0::ExecutionUnit> executionUnit, taskEventMap_t *eventMap = NULL)
    : _label(label),
      _executionUnit(executionUnit),
      _eventMap(eventMap){};

  /**
   * Function to return a pointer to the currently executing task from a global context
   *
   * @return A pointer to the current HiCR task, NULL if this function is called outside the context of a task run() function
   */
  __INLINE__ static Task *getCurrentTask() { return (Task *)pthread_getspecific(_taskPointerKey); }

  /**
   * Sets the task's event map. This map will be queried whenever a state transition occurs, and if the map defines a callback for it, it will be executed.
   *
   * @param[in] eventMap A pointer to an event map
   */
  __INLINE__ void setEventMap(taskEventMap_t *eventMap) { _eventMap = eventMap; }

  /**
   * Gets the task's event map.
   *
   * @return A pointer to the task's an event map. NULL, if not defined.
   */
  __INLINE__ taskEventMap_t *getEventMap() { return _eventMap; }

  /**
   * Sends a sync signal, triggering the associated event
   */
  __INLINE__ void sendSyncSignal() { _eventMap->trigger(this, HiCR::tasking::Task::event_t::onTaskSync); }

  /**
   * Queries the task's internal state.
   *
   * @return The task internal state
   *
   * \internal This is not a thread safe operation.
   */
  __INLINE__ const HiCR::L0::ExecutionState::state_t getState()
  {
    // If the execution state has not been initialized then return the value expliclitly
    if (_executionState == NULL) return HiCR::L0::ExecutionState::state_t::uninitialized;

    // Otherwise just query the initial execution state
    return _executionState->getState();
  }

  /**
   * Sets the execution unit assigned to this task
   *
   * \param[in] executionUnit The execution unit to assign to this task
   */
  __INLINE__ void setExecutionUnit(std::shared_ptr<HiCR::L0::ExecutionUnit> executionUnit) { _executionUnit = executionUnit; }

  /**
   * Returns the execution unit assigned to this task
   *
   * \return The execution unit assigned to this task
   */
  __INLINE__ std::shared_ptr<HiCR::L0::ExecutionUnit> getExecutionUnit() const { return _executionUnit; }

  /**
   * Implements the initialization routine of a task, that stores and initializes the execution state to run to completion
   *
   * \param[in] executionState A previously initialized execution state
   */
  __INLINE__ void initialize(std::unique_ptr<HiCR::L0::ExecutionState> executionState)
  {
    if (getState() != HiCR::L0::ExecutionState::state_t::uninitialized)
      HICR_THROW_LOGIC("Attempting to initialize a task that has already been initialized (State: %d).\n", getState());

    // Getting execution state as a unique pointer (to prevent sharing the same state among different tasks)
    _executionState = std::move(executionState);
  }

  /**
   * This function starts running a task. It needs to be performed by a worker, by passing a pointer to itself.
   *
   * The execution of the task will trigger change of state from initialized to running. Before reaching the terminated state, the task might transition to some of the suspended states.
   */
  __INLINE__ void run()
  {
    // Preventing this function from being accessed simultaneously (that would be a bug in the callers code)
    _mutex.lock();

    if (_isInitialized == false) HICR_THROW_RUNTIME("HiCR Tasking functionality was not yet initialized");

    if (getState() != HiCR::L0::ExecutionState::state_t::initialized && getState() != HiCR::L0::ExecutionState::state_t::suspended)
      HICR_THROW_RUNTIME("Attempting to run a task that is not in a initialized or suspended state (State: %d).\n", getState());

    // Also map task pointer to the running thread it into static storage for global access.
    pthread_setspecific(_taskPointerKey, this);

    // Triggering execution event, if defined
    if (_eventMap != NULL) _eventMap->trigger(this, event_t::onTaskExecute);

    // Now resuming the task's execution
    _executionState->resume();

    // Checking execution state finalization
    _executionState->checkFinalization();

    // Getting state after execution
    const auto state = getState();

    // If the task is suspended and event map is defined, trigger the corresponding event.
    if (state == HiCR::L0::ExecutionState::state_t::suspended)
      if (_eventMap != NULL) _eventMap->trigger(this, event_t::onTaskSuspend);

    // If the task is still running (no suspension), then the task has fully finished executing. If so,
    // trigger the corresponding event, if the event map is defined. It is important that this function
    // is called from outside the context of a task to allow the upper layer to free its memory upon finishing
    if (state == HiCR::L0::ExecutionState::state_t::finished)
      if (_eventMap != NULL) _eventMap->trigger(this, event_t::onTaskFinish);

    // Relenting current task pointer
    pthread_setspecific(_taskPointerKey, NULL);

    // Releasing lock
    _mutex.unlock();
  }

  /**
   * This function yields the execution of the task, and returns to the worker's context.
   */
  __INLINE__ void suspend()
  {
    if (getState() != HiCR::L0::ExecutionState::state_t::running) HICR_THROW_RUNTIME("Attempting to yield a task that is not in a running state (State: %d).\n", getState());

    // Since this function is public, it can be called from anywhere in the code. However, we need to make sure on rutime that the context belongs to the task itself.
    if (getCurrentTask() != this) HICR_THROW_RUNTIME("Attempting to yield a task from a context that is not its own.\n");

    // Yielding execution back to worker
    _executionState->suspend();
  }

  /**
   * Returns the task's label
   *
   * \return The task's label
   */
  __INLINE__ label_t getLabel() const { return _label; }

  /**
   * Adds an execution depedency to this task. This means that this task will not be ready to execute until and unless
   * the task referenced by this label has finished executing.
   *
   * \param[in] task The label of the task upon whose completion this task should depend
   */
  __INLINE__ void addTaskDependency(const label_t task) { _taskDependencies.push_back(task); };

  /**
   * Returns Returns this task's dependency list.
   *
   * \return A constant reference to this task's dependencies vector.
   */
  __INLINE__ const std::vector<label_t> &getDependencies() { return _taskDependencies; }

  private:

  /**
   * Tasks's label, chosen by the user
   */
  const label_t _label;

  /**
   * Task execution dependency list. The task will be ready only if this container is empty.
   */
  std::vector<label_t> _taskDependencies;

  /**
   * Execution unit that will be instantiated and executed by this task
   */
  std::shared_ptr<HiCR::L0::ExecutionUnit> _executionUnit;

  /**
   *  Map of events to trigger
   */
  taskEventMap_t *_eventMap = NULL;

  /**
   * Internal execution state of the task. Will change based on runtime scheduling events
   */
  std::unique_ptr<HiCR::L0::ExecutionState> _executionState = NULL;

  /**
   * Mutex to prevent task from being executed concurrently
  */
  std::mutex _mutex;

}; // class Task

} // namespace tasking

} // namespace HiCR
