#pragma once

#include <vector>
#include <hicr/task.hpp>
#include <hicr/common.hpp>

namespace HiCR
{

class TaskPool
{
 private:

 // Lock-free queue for waiting tasks
 lockFreeQueue_t<Task*, MAX_SIMULTANEOUS_TASKS> _queue;

 public:

 inline void dispatchTask(Task* task, void* arg)
 {
  task->setArgument(arg);
  _queue.push(task);
 }

 inline Task* getNextTask()
 {
  Task* task = NULL;

  // Poping next task from the lock-free queue
  _queue.try_pop(task);

  // If we did not find a ready task, then return a NULL pointer
  return task;
 }

 TaskPool() = default;
 ~TaskPool() = default;
};


} // namespace HiCR
