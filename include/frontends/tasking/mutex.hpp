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

  __USED__ inline void lock()
  {
    auto currentTask = HiCR::tasking::Task::getCurrentTask();
    lockImpl(nullptr, currentTask);
  }

  __USED__ inline void unlock()
  {
    auto currentTask = HiCR::tasking::Task::getCurrentTask();
    lockImpl(currentTask, nullptr);
  }

  private:

  __USED__ inline void lockImpl(HiCR::tasking::Task* expected, HiCR::tasking::Task* desired)
  {
    bool success = false;

    while(success == false)
    {
      auto e = expected;
      success = _lockValue.compare_exchange_weak(e, desired);
      if (success == false) HiCR::tasking::Task::getCurrentTask()->suspend();
    }
  }

  std::atomic<HiCR::tasking::Task*> _lockValue = nullptr;
};

} // namespace tasking

} // namespace HiCR