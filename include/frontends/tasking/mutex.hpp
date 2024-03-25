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
    auto currentTask = HiCR::tasking::Task::getCurrentTask();
    lockBlockingImpl(currentTask, nullptr);
  }

  private:

  __USED__ inline void lockBlockingImpl(HiCR::tasking::Task* expected, HiCR::tasking::Task* desired)
  {
    bool success = false;

    while(success == false)
    {
      success = lockNotBlockingImpl(expected, desired);
      if (success == false) HiCR::tasking::Task::getCurrentTask()->suspend();
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