#pragma once

#include <taskr/runtime.hpp>
#include <taskr/task.hpp>

namespace taskr
{

typedef uint64_t workerId_t;

class Worker
{
private:

 workerId_t _workerId;
 Task* _currentTask;

public:

 inline Worker()
 {
  _workerId = pthread_self();
 }

 inline bool checkReadyTasks()
 {
  // Pointer to the next task to execute
  Task* task;

  // Poping next task from the lock-free queue
  bool foundTask = _runtime->_readyTaskQueue.try_pop(task);

  // If we did not find a ready task, then either all are being executed or all tasks are waiting (or a combination thereof)
  if (foundTask == false) return false;

  // If a task was found (queue was not empty), then execute and manage the task depending on its state
  _currentTask = task;
  task->run();

  // Decreasing overall task count
  _runtime->_taskCount--;

  // Adding task label to finished task set
  _runtime->_finishedTaskHashMap.insert(task->getLabel());

  // Free task's memory to prevent leaks. Could not use unique_ptr because the type is not supported by boost's lock-free queue
  delete task;

  return true;
 }

 // This function finds a task in the waiting queue and checks its dependencies
 inline bool checkWaitingTasks() const
 {
  // Pointer to the next task to execute
  Task* task;

  // Poping next task from the lock-free queue
  bool foundTask = _runtime->_waitingTaskQueue.try_pop(task);

  // If a task was found (queue was not empty), then execute and manage the task depending on its state
  if (foundTask == false) return false;

  // Check if task is ready now
  bool isTaskReady = task->isReady();

  // If it is, put it in the ready-to-go queue
  if (isTaskReady == true) _runtime->_readyTaskQueue.push(task);

  // Otherwise, back into the waiting task pile
  if (isTaskReady == false) _runtime->_waitingTaskQueue.push(task);

  return true;
 }

 inline void run()
 {
  // Run tasks until all of them are finished
  while (_runtime->_taskCount > 0)
  {
   // Run ready tasks until none of them remain
   while(checkReadyTasks());

   // When no more ready tasks remain, use the worker to check the dependencies of those waiting (if any)
   checkWaitingTasks();
  }
 }

 inline workerId_t getWorkerId() { return _workerId; }
 inline Task* getCurrentTask() { return _currentTask; }

}; // class Worker

} // namespace taskr



