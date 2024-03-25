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
#include <thread>
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

  void wait()
  {
     auto currentTask = HiCR::tasking::Task::getCurrentTask();
     bool keepWaiting = true;
 
    _mutex.lock();
    _waitingTasks.insert(currentTask);
    _mutex.unlock();

    while (keepWaiting == true)
    {
      currentTask->suspend();
      _mutex.lock();
      keepWaiting = _waitingTasks.contains(currentTask);
      _mutex.unlock();
    }
  }

  void notifyOne()
  {
    _mutex.lock();
    _waitingTasks.erase(_waitingTasks.begin());
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