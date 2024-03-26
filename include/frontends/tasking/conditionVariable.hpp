/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file conditionVariable.hpp
 * @brief Provides a class definition for a task-aware condition variable object
 * @author S. M. Martin
 * @date 25/3/2024
 */

#pragma once

#include <set>
#include "mutex.hpp"
#include "task.hpp"

namespace HiCR
{

namespace tasking
{

class ConditionVariable
{
  public:

  ConditionVariable() = default;
  ~ConditionVariable() = default;

  void wait(tasking::Mutex& conditionMutex, std::function<bool(void)> conditionFunction)
  {
    auto currentTask = HiCR::tasking::Task::getCurrentTask();
 
    // Checking on the condition
    conditionMutex.lock();
    bool keepWaiting = conditionFunction() == false;
    conditionMutex.unlock();

    // If the condition is not satisfied, suspend until we're notified and the condition is satisfied
    if (keepWaiting == true)
    {
      // Insert oneself in the waiting task list
      _mutex.lock();
      _waitingTasks.insert(currentTask);
      _mutex.unlock();

      // Register a pending operation that will prevent task from being rescheduled until finished
      currentTask->registerPendingOperation([&]()
      {
        // Checking whether this task has been notified
        _mutex.lock();
        bool isNotified = _waitingTasks.contains(currentTask) == false;
        _mutex.unlock();

        // If not notified, re-add task to notification list and stop evaluating
        if (isNotified == false)
        {
          _mutex.lock();
          _waitingTasks.insert(currentTask);
          _mutex.unlock();
          return false;
        } 

        // Checking actual condition
        conditionMutex.lock();
        bool isConditionSatisfied = conditionFunction();
        conditionMutex.unlock();

        // Return whether the condition is satisfied 
        return isConditionSatisfied;
      });

      // Suspending task
      currentTask->suspend();
    }
  }

  void notifyOne()
  {
    _mutex.lock();
    if (_waitingTasks.empty() == false) _waitingTasks.erase(_waitingTasks.begin());
    _mutex.unlock();
  }

  void notifyAll()
  {
    _mutex.lock();
    _waitingTasks.clear();
    _mutex.unlock();
  }

  private:

  tasking::Mutex _mutex;

  std::set<HiCR::tasking::Task*> _waitingTasks;
};

} // namespace tasking

} // namespace HiCR