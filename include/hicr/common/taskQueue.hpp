#pragma once

#include <hicr/common/definitions.hpp>
#include <../../../extern/atomic_queue/atomic_queue.h>

namespace HiCR {

class Task;

namespace common {

// Implementation of the lockfree queue
template <class T, unsigned int N> using lockFreeQueue_t = atomic_queue::AtomicQueue<T, N, (T)NULL>;

class taskQueue
{
 private:

 // Internal implementation of the task queue
 lockFreeQueue_t<Task*, MAX_QUEUED_TASKS> _queue;

 public:

 inline void push(Task* task)
 {
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
};

} // namespace common

} // namespace HiCR
