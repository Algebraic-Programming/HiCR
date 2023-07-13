#pragma once

#include <hicr/logger.hpp>
#include <hicr/common.hpp>
#include <hicr/coroutine.hpp>
#include <hicr/event.hpp>

namespace HiCR
{

// Definition for task unique identifiers
typedef uint64_t taskId_t;

// Definition for a task function - supports lambda functions
typedef coroutineFc_t taskFunction_t;

enum task_state {
                  ready, // Ready to run -- set automatically upon creation
                  running, // Set by the worker upon execution
                  waiting, // Set by the task if it suspends for an asynchronous operation
                  finished, // Set by the task upon complete termination
                };

class Task
{
 public:

  Task(taskFunction_t fc) : _fc{fc} {}
  ~Task() = default;

  inline const task_state getState() { return _state; };

  inline void run()
  {
   if (_state != task_state::ready) LOG_ERROR("Attempting to run a task that is not in a ready state (State: %d).\n", _state);

   // Setting state to running while we execute
   _state = task_state::running;

   // Triggering execution event, if defined
   if( _eventMap != NULL && _eventMap->contains(event_t::onTaskExecute)) (*_eventMap)[event_t::onTaskExecute]->trigger(this);

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

   // If the state is still running (no suspension or yield), then the task has finished executing
   if (_state == task_state::running) _state = task_state::finished;

   // Triggering events, if defined
   if (_eventMap != NULL) switch(_state)
   {
     case task_state::running:  break;
     case task_state::finished: if (_eventMap->contains(event_t::onTaskFinish)) (*_eventMap)[event_t::onTaskFinish]->trigger(this);  break;
     case task_state::ready:    if (_eventMap->contains(event_t::onTaskYield))  (*_eventMap)[event_t::onTaskYield]->trigger(this);   break;
     case task_state::waiting:  if (_eventMap->contains(event_t::onTaskSuspend))(*_eventMap)[event_t::onTaskSuspend]->trigger(this); break;
   }
  }

 inline void yield()
 {
   // Change our state to yielded so that we can be reinserted into the pool
   _state = task_state::ready;

   // Yielding execution back to worker
   _coroutine.yield();
 }

 inline void setArgument(void* argument) { _argument = argument; }
 inline void setEventMap(eventMap_t* eventMap) { _eventMap = eventMap; }

 private:

  // Current execution state of the task. Will change based on runtime scheduling events
  task_state _state = task_state::ready;

  // Remember if the task has been executed already (coroutine still exists)
  bool _hasExecuted = false;

  // Argument to execute the task with
  void* _argument;

  // Main function that the task will execute
  taskFunction_t _fc;

  // Task context preserved as a coroutine
  Coroutine _coroutine;

  // Map of events to trigger
  eventMap_t* _eventMap = NULL;
};

} // namespace HiCR

