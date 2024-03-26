/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file mutex.hpp
 * @brief Provides a class definition for a task-aware mutex
 * @author S. M. Martin
 * @date 25/3/2024
 */

#pragma once

#include <atomic>
#include <hicr/exceptions.hpp>
#include "task.hpp"

namespace HiCR
{

namespace tasking
{

class Mutex
{
  public:

  Mutex() = default;
  ~Mutex() = default;

  __USED__ inline bool ownsLock()
  {
    auto currentTask = HiCR::tasking::Task::getCurrentTask();
    return _lockValue.load() == currentTask;
  }

  __USED__ inline bool trylock()
  {
    auto currentTask = HiCR::tasking::Task::getCurrentTask();
    return lockNotBlockingImpl(nullptr, currentTask);
  }

  __USED__ inline void lock()
  {
    auto currentTask = HiCR::tasking::Task::getCurrentTask();
    lockBlockingImpl(nullptr, currentTask);
  }

  __USED__ inline void unlock()
  {
    if (ownsLock() == false) HICR_THROW_LOGIC("Trying to unlock a mutex that doesn't belong to this task");
    _lockValue = nullptr;
  }

  private:

  __USED__ inline void lockBlockingImpl(HiCR::tasking::Task* expected, HiCR::tasking::Task* desired)
  {
    // Getting current task
    auto currentTask = HiCR::tasking::Task::getCurrentTask();

    // Checking right away if we can get the lock
    bool success = lockNotBlockingImpl(expected, desired);
    
    // If not successful, then suspend the task
    if (success == false) 
    {
      // Register a pending operation that will prevent task from being rescheduled until finished
      currentTask->registerPendingOperation([&]() { return lockNotBlockingImpl(expected, desired); });

      // Prevent from re-executing task until the lock is obtained
      currentTask->suspend();
    }
  }

  __USED__ inline bool lockNotBlockingImpl(HiCR::tasking::Task* expected, HiCR::tasking::Task* desired)
  {
    return _lockValue.compare_exchange_weak(expected, desired);
  }

  std::atomic<HiCR::tasking::Task*> _lockValue = nullptr;
};

} // namespace tasking

} // namespace HiCR