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

  Task(taskFunction_t fc) : _fc(fc) {}
  ~Task() = default;

  inline void setArgument(void* argument) { _argument = argument; }
  inline task_state getState() { return _state; }
  inline taskId_t getId() { return _id; }

 private:

  // Current execution state of the task. Will change based on runtime scheduling events
  task_state _state = task_state::initial;

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

