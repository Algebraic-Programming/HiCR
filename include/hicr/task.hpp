#pragma once

#include <hicr/logger.hpp>
#include <hicr/common.hpp>
#include <hicr/event.hpp>
#include <hicr/coroutine.hpp>

namespace HiCR
{

enum task_state { initial, dispatched, running, waiting, finished };

class Task
{
 public:

  Task(taskId_t id, taskFunction_t fc) : _id {id}, _fc{fc} {}
  ~Task() = default;

  inline void*& argument() { return _argument; }
  inline task_state& state() { return _state; }
  inline taskId_t& id() { return _id; }

  inline void setTerminal() { _isTerminal = true; }
  inline bool isTerminal() { return _isTerminal; }

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
   _state = task_state::finished;
  }

 private:

  // Current execution state of the task. Will change based on runtime scheduling events
  task_state _state = task_state::initial;

  // This flag indicates that the executing worker should also terminate upon its completion
  bool _isTerminal = false;

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

