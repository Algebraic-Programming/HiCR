#pragma once

#include <hicr/common/logger.hpp>
#include <hicr/coroutine.hpp>
#include <hicr/eventMap.hpp>

namespace HiCR
{

// Definition for task unique identifiers
typedef uint64_t taskId_t;

// Definition for a task function - supports lambda functions
typedef coroutineFc_t taskFunction_t;

class Worker;

enum task_state
{
  ready,    // Ready to run -- set automatically upon creation
  running,  // Set by the worker upon execution
  waiting,  // Set by the task if it suspends for an asynchronous operation
  finished, // Set by the task upon complete termination
};

class Task
{
  public:
  inline void setFunction(taskFunction_t fc) { _fc = fc; }
  inline void setArgument(void *argument) { _argument = argument; }
  inline void setEventMap(EventMap *eventMap) { _eventMap = eventMap; }
  inline const task_state getState() { return _state; };
  inline void *getArgument() { return _argument; }
  const inline Worker *getWorker() { return _worker; }

  // Start running a task, this can only be done by a worker
  inline void run(Worker *worker)
  {
    if (_state != task_state::ready) LOG_ERROR("Attempting to run a task that is not in a ready state (State: %d).\n", _state);

    // Storing worker
    _worker = worker;

    // Setting state to running while we execute
    _state = task_state::running;

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
    if (_state == task_state::running) _state = task_state::finished;

    // Triggering events, if defined
    if (_eventMap != NULL) switch (_state)
      {
      case task_state::running: break;
      case task_state::finished: _eventMap->trigger(this, event_t::onTaskFinish); break;
      case task_state::ready: _eventMap->trigger(this, event_t::onTaskYield); break;
      case task_state::waiting: _eventMap->trigger(this, event_t::onTaskSuspend); break;
      }
  }

  private:
  // Yield must be called by the task itself, not by external agents.
  inline void yield()
  {
    // Change our state to yielded so that we can be reinserted into the pool
    _state = task_state::ready;

    // Yielding execution back to worker
    _coroutine.yield();
  }

  // Current execution state of the task. Will change based on runtime scheduling events
  task_state _state = task_state::ready;

  // Remember if the task has been executed already (coroutine still exists)
  bool _hasExecuted = false;

  // Argument to execute the task with
  void *_argument;

  // Main function that the task will execute
  taskFunction_t _fc;

  // Task context preserved as a coroutine
  Coroutine _coroutine;

  // Map of events to trigger
  EventMap *_eventMap = NULL;

  // Worker currently executing the task
  Worker *_worker = NULL;
};

} // namespace HiCR
