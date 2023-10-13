/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file task.hpp
 * brief Provides a definition for the HiCR Task class.
 * @author S. M. Martin
 * @date 7/7/2023
 */

#pragma once

#include <queue>
#include <memory>
#include <hicr/common/coroutine.hpp>
#include <hicr/common/definitions.hpp>
#include <hicr/common/eventMap.hpp>
#include <hicr/common/exceptions.hpp>
#include <hicr/executionUnit.hpp>
#include <hicr/executionState.hpp>


namespace HiCR
{

class Task;

/**
 * Static storage for remembering the executing worker and task, based on the pthreadId
 *
 * \note Be mindful of possible destructive interactions between this thread local storage and coroutines.
 *       If this fails at some point, it might be necessary to come back to a pthread_self() mechanism
 */
thread_local Task *_currentTask;

/**
 * Function to return a pointer to the currently executing task from a global context
 *
 * @return A pointer to the current HiCR task, NULL if this function is called outside the context of a task run() function
 */
__USED__ inline Task *getCurrentTask() { return _currentTask; }

/**
 * This class defines the basic execution unit managed by HiCR.
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
    onTaskFinish
  };

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
   * Type definition for the task's event map
   */
  typedef common::EventMap<Task, event_t> taskEventMap_t;

  /**
   * Definition for a task function that supports lambda functions
   */
  typedef common::Coroutine::coroutineFc_t taskFunction_t;

  /**
   * Definition for a task function that register operations that have been started by the task but not yet finalized
   *
   * Running the function must return True, if the operation has finished, and; False, if the operation finished
   */
  typedef std::function<bool()> pendingOperationFunction_t;

  /**
   * Definition for the collection of pending operations
   */
  typedef std::queue<pendingOperationFunction_t> pendingOperationFunctionQueue_t;

  Task() = delete;
  ~Task() = default;

  /**
   * Constructor that sets the function that the task will execute and its argument.
   *
   * @param[in] fc Specifies the function to execute.
   * @param[in] eventMap Pointer to the event map callbacks to be called by the task
   */
  __USED__ Task(const ExecutionUnit* executionUnit, void *argument = NULL, taskEventMap_t *eventMap = NULL) : _executionUnit(executionUnit), _eventMap(eventMap){};

  /**
   * Sets the task's event map. This map will be queried whenever a state transition occurs, and if the map defines a callback for it, it will be executed.
   *
   * @param[in] eventMap A pointer to an event map
   */
  __USED__ inline void setEventMap(taskEventMap_t *eventMap) { _eventMap = eventMap; }

  /**
   * Gets the task's event map.
   *
   * @return A pointer to the task's an event map. NULL, if not defined.
   */
  __USED__ inline taskEventMap_t *getEventMap() { return _eventMap; }

  /**
   * Queries the task's internal state.
   *
   * @return The task internal state
   *
   * \internal This is not a thread safe operation.
   */
  __USED__ inline const state_t getState() { return _state; }


  __USED__ inline const ExecutionUnit* getExecutionUnit() { return _executionUnit; }

  /**
   * Registers an operation that has been started by the task but has not yet finished
   *
   * @param[in] op The pending operation
   */
  __USED__ inline void registerPendingOperation(pendingOperationFunction_t op)
  {
    _pendingOperations.push(op);
  }

  /**
   * Checks for the finalization of all the task's pending operations and reports whether they have finished
   *
   * Operations that have finished are removed from the Task's storage. These will be removed, even if some might remain and the return is false
   *
   * @return True, if the task no longer contains pending operations; false, if pending operations remain.
   */
  __USED__ inline bool checkPendingOperations()
  {
    while (_pendingOperations.empty() == false)
    {
      // Getting the pending function
      const auto &fc = _pendingOperations.front();

      // Running it to see whether it has finished
      bool finished = fc();

      // If it hasn't, return false immediately
      if (finished == false) return false;

      // Otherwise, remove current element
      _pendingOperations.pop();
    }

    // No pending operations remain, return true
    return true;
  }

  __USED__ inline void initialize(std::unique_ptr<ExecutionState> executionState)
  {
    if (_state != state_t::uninitialized) HICR_THROW_LOGIC("Attempting to initialize a task that has already been initialized (State: %d).\n", _state);

    // Getting execution state as a unique pointer (to prevent sharing the same state among different tasks)
    _executionState = std::move(executionState);

    // Setting state as initialized
    _state = state_t::initialized;
  }

  /**
   * This function starts running a task. It needs to be performed by a worker, by passing a pointer to itself.
   *
   * The execution of the task will trigger change of state from initialized to running. Before reaching the terminated state, the task might transition to some of the suspended states.
   */
  __USED__ inline void run()
  {
    if (_state != state_t::initialized && _state != state_t::suspended) HICR_THROW_RUNTIME("Attempting to run a task that is not in a initialized or suspended state (State: %d).\n", _state);

    // Also map task pointer to the running thread it into static storage for global access.
    _currentTask = this;

    // Setting state to running while we execute
    _state = state_t::running;

    // Triggering execution event, if defined
    if (_eventMap != NULL) _eventMap->trigger(this, event_t::onTaskExecute);

    // Now resuming the task's execution
    _executionState->resume();

    // If the task is suspended and event map is defined, trigger the corresponding event.
    if (_state == state_t::suspended)
      if (_eventMap != NULL) _eventMap->trigger(this, event_t::onTaskSuspend);

    // If the task is still running (no suspension), then the task has fully finished executing
    if (_state == state_t::running)
    {
      // Setting state as finished
      _state = state_t::finished;

      // Trigger the corresponding event, if the event map is defined. It is important that this function is called from outside the context of a task to allow the upper layer to free its memory upon finishing
      if (_eventMap != NULL) _eventMap->trigger(this, event_t::onTaskFinish);
    }

    // Relenting current task pointer
    _currentTask = NULL;
  }

  /**
   * This function yields the execution of the task, and returns to the worker's context.
   */
  __USED__ inline void yield()
  {
    if (_state != state_t::running) HICR_THROW_RUNTIME("Attempting to yield a task that is not in a running state (State: %d).\n", _state);

    // Since this function is public, it can be called from anywhere in the code. However, we need to make sure on rutime that the context belongs to the task itself.
    if (getCurrentTask() != this) HICR_THROW_RUNTIME("Attempting to yield a task from a context that is not its own.\n");

    // Change our state to yielded so that we can be reinserted into the pool
    _state = state_t::suspended;

    // Yielding execution back to worker
    _executionState->yield();
  }

  private:

  /**
   * Current execution state of the task. Will change based on runtime scheduling events
   */
  state_t _state = state_t::uninitialized;

  const ExecutionUnit* const _executionUnit;

  /**
   *  Map of events to trigger
   */
  taskEventMap_t *_eventMap = NULL;

  /**
   * List of pending operations initiated by the task but not yet finished
   */
  pendingOperationFunctionQueue_t _pendingOperations;

  std::unique_ptr<ExecutionState> _executionState;
};

} // namespace HiCR
