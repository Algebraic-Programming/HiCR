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

    // If the condition is not satisfied:
    while (keepWaiting == true)
    {
      // Insert oneself in the waiting task list
      _mutex.lock();
      _waitingTasks.insert(currentTask);
      _mutex.unlock();

      // Suspend execution
      currentTask->suspend();

      // Do not wake up until notified
      bool isNotified = false;
      while(isNotified == false)
      {
        _mutex.lock();
        isNotified = _waitingTasks.contains(currentTask) == false;
        _mutex.unlock();
      }

      // Now we were notified, get condition lock and check condition again
      conditionMutex.lock();
      keepWaiting = conditionFunction() == false;
      conditionMutex.unlock();
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