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
    HiCR::tasking::Task* expected;

    do 
    {
      expected = nullptr;
      _lockValue.compare_exchange_weak(expected, currentTask);
    } while(expected != currentTask);
  }

  __USED__ inline void unlock()
  {
    HiCR::tasking::Task* expected;
    auto currentTask = HiCR::tasking::Task::getCurrentTask();

    do 
    {
      expected = currentTask;
      _lockValue.compare_exchange_weak(expected, nullptr);
    } while(expected != nullptr);
  }

  private:

  std::atomic<HiCR::tasking::Task*> _lockValue = nullptr;
};

} // namespace tasking

} // namespace HiCR