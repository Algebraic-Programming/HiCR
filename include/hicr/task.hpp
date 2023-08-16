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

#include <hicr/common/logger.hpp>
#include <hicr/common/coroutine.hpp>
#include <hicr/eventMap.hpp>

namespace HiCR
{

class Worker;

/**
 * Definition for a task function that supports lambda functions
 */
typedef coroutineFc_t taskFunction_t;

namespace task
{

/**
 * Complete state set that a task can be in
 */
enum state_t
{
 /**
  * Ready to run -- set automatically upon creation
  */
  ready,

  /**
   * Indicates that the task is currently running
   */
  running,

  /**
   * Set by the task if it suspends for an asynchronous operation
   */
  waiting,

  /**
   * Set by the task upon complete termination
   */
  finished
};

} // namespace task

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
  * Sets the function that the task will execute.
  *
  * @param[in] fc Speficies the function to execute.
  */
  inline void setFunction(taskFunction_t fc) { _fc = fc; }

  /**
   * Sets the single argument (pointer) to the the task function
   *
   * @param[in] argument A pointer representing the function's argument
   */
  inline void setArgument(void *argument) { _argument = argument; }

  /**
   * Sets the task's event map. This map will be queried whenever a state transition occurs, and if the map defines a callback for it, it will be executed.
   *
   * @param[in] eventMap A pointer to an event map
   */
  inline void setEventMap(EventMap *eventMap) { _eventMap = eventMap; }

  /**
   * Queries the task's internal state.
   *
   * @return The task internal state
   *
   * \internal This is not a thread safe operation.
   */
  inline const task::state_t getState() { return _state; }

  /**
   * Queries the task's function argument.
   *
   * @return A pointer user-defined task argument, if defined; A NULL pointer, if not.
   */
  inline void *getArgument() { return _argument; }

  /**
   * Returns the worker that is currently executing this task. This call only makes sense if the task is in the 'Running' state.
   *
   * @return A pointer to the worker running this task, if running; a NULL pointer, if not.
   */
  const inline Worker *getWorker() { return _worker; }

  /**
   * This function starts running a task. It needs to be performed by a worker, by passing a pointer to itself.
   *
   * The execution of the task will trigger change of state from ready to running. Before reaching the terminated state, the task might transition to some of the suspended states.
   *
   * @param[in] worker A pointer to the worker that is calling this function.
   */
  inline void run(Worker *worker)
  {
    if (_state != task::state_t::ready) LOG_ERROR("Attempting to run a task that is not in a ready state (State: %d).\n", _state);

    // Storing worker
    _worker = worker;

    // Setting state to running while we execute
    _state = task::state_t::running;

    // Triggering execution event, if defined
    if (_eventMap != NULL) _eventMap->trigger(this, event_t::onTaskExecute);

    // If this is the first time we execute this task, we create the new coroutine, otherwise resume the already created one
    if (_hasExecuted == false)
    {
      _coroutine.start(_fc, _argument);
      _hasExecuted = true;
    }
    else
    {
      _coroutine.resume();
    }

    // After returning, the task is no longer associated to a worker
    _worker = NULL;

    // If the state is still running (no suspension or yield), then the task has finished executing
    if (_state == task::state_t::running) _state = task::state_t::finished;

    // Triggering events, if defined
    if (_eventMap != NULL) switch (_state)
      {
      case task::state_t::running: break;
      case task::state_t::finished: _eventMap->trigger(this, event_t::onTaskFinish); break;
      case task::state_t::ready: _eventMap->trigger(this, event_t::onTaskYield); break;
      case task::state_t::waiting: _eventMap->trigger(this, event_t::onTaskSuspend); break;
      }
  }

  private:

  /**
   * This function yields the execution of the task, and returns to the worker's context.
   */
  inline void yield()
  {
    // Change our state to yielded so that we can be reinserted into the pool
    _state = task::state_t::ready;

    // Yielding execution back to worker
    _coroutine.yield();
  }

  /**
   * Current execution state of the task. Will change based on runtime scheduling events
   */
  task::state_t _state = task::state_t::ready;

  /**
   *  Remember if the task has been executed already (coroutine still exists)
   */
  bool _hasExecuted = false;

  /**
   *   Argument to execute the task with
   */
  void *_argument;

  /**
   *  Main function that the task will execute
   */
  taskFunction_t _fc;

  /**
   *  Task context preserved as a coroutine
   */
  Coroutine _coroutine;

  /**
   *  Map of events to trigger
   */
  EventMap *_eventMap = NULL;

  /**
   * Worker currently executing the task
   */
  Worker *_worker = NULL;
};

} // namespace HiCR
