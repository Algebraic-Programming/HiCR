#pragma once

#include <hicr/logger.hpp>
#include <hicr/common.hpp>
#include <hicr/event.hpp>
#include <hicr/coroutine.hpp>

namespace HiCR
{

enum task_state { initial, // Set automatically upon creation
                  dispatched, // Set by the task pool when dispatched
                  running, // Set by the worker upon execution
                  waiting, // Set by the task if it suspends for an asynchronous operation
                  yielded, // Set by the task if it suspends voluntarily (is re-added to the task pool)
                  finished, // Set by the task upon complete termination
                  terminal // Set by the task if this is a terminal state (terminates the worker too)
                };

class Task
{
 public:

  Task(taskId_t id, taskFunction_t fc) : _id {id}, _fc{fc} {}
  ~Task() = default;

  inline task_state& state() { return _state; }
  inline taskId_t& id() { return _id; }

  inline void run()
  {
   if (_state != task_state::dispatched) LOG_ERROR("Attempting to run a task that is not in a dispatched state (State: %d).\n", _state);

   // Setting state to running while we execute
   _state = task_state::running;

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

   // Task has ran to completion
   if (_state == task_state::running) _state = task_state::finished;
  }

 inline void yield()
 {
   // Change our state to yielded so that we can be reinserted into the pool
   _state = task_state::yielded;

   // Yielding execution back to worker
   _coroutine.yield();
 }

 inline void terminate()
 {
  // Change our state to terminal, to indicate that the worker should also finish
  _state = task_state::terminal;

  // Yielding execution back to worker
  _coroutine.yield();
 }

 inline void setArgument(void* argument) { _argument = argument; }

 private:

  // Current execution state of the task. Will change based on runtime scheduling events
  task_state _state = task_state::initial;

  // Remember if the task has been executed already (coroutine still exists)
  bool _hasExecuted = false;

  // Argument to execute the task with
  void* _argument;

  // Identifier for the task, might not be unique
  taskId_t _id;

  // Main function that the task will execute
  taskFunction_t _fc;

  // Task context preserved as a coroutine
  Coroutine _coroutine;
};

} // namespace HiCR

