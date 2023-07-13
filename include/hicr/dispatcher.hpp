#pragma once

#include <vector>
#include <hicr/task.hpp>
#include <hicr/common.hpp>
#include <hicr/event.hpp>

namespace HiCR
{

typedef std::function<Task*(void)> pullFunction_t;

class Dispatcher
{
 private:

 // Lock-free queue for waiting tasks
 lockFreeQueue_t<Task*, MAX_SIMULTANEOUS_TASKS> _queue;

 // Pull function, if defined by the user
 pullFunction_t _pullFc;
 bool _isPullFcDefined = false;

 public:

 inline void clearPullFunction()
 {
   _isPullFcDefined = false;
 }

 inline void setPullFunction(const pullFunction_t pullFc)
 {
  _pullFc = pullFc;
  _isPullFcDefined = true;
 }

 inline void push(Task* task)
 {
  if (task->getState() != task_state::ready) LOG_ERROR("Attempting to dispatch a task that is not in a initial state (State: %d).\n", task->getState());

  _queue.push(task);
 }

 inline Task* pop()
 {
  Task* task = NULL;

  // Poping next task from the lock-free queue
  _queue.try_pop(task);

  // If we did not find a ready task, then return a NULL pointer
  return task;
 }

 inline size_t wasSize() const
 {
  return _queue.was_size();
 }

 inline Task* popPull()
 {
  // First, attempting to pull
  Task* task = this->pop();

  // If there are no tasks in the queue and the pull function is defined, then call it
  if (task == NULL && _isPullFcDefined == true) task = this->_pullFc();

  return task;
 }

 Dispatcher() = default;
 ~Dispatcher() = default;
};


} // namespace HiCR
