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

/**
 * Implementation of a HiCR task-aware mutual exclusion mechanism
*/
class Mutex
{
  public:

  Mutex()  = default;
  ~Mutex() = default;

  /**
   * Checks whether a given task owns the lock
   * 
   * \param[in] task The task to check ownership for
   * @return True, if the task owns the lock; false, otherwise.
  */
  __USED__ inline bool ownsLock(HiCR::tasking::Task *task = HiCR::tasking::Task::getCurrentTask()) { return _lockValue.load() == task; }

  /**
   * Tries to obtain the lock and returns immediately if it fails.
   *
   * \param[in] task The task acquiring the lock
   * @return True, if it succeeded in obtaining the lock; false, otherwise.
  */
  __USED__ inline bool trylock(HiCR::tasking::Task *task = HiCR::tasking::Task::getCurrentTask()) { return lockNotBlockingImpl(task); }

  /**
   * Obtains the mutual exclusion lock.
   * 
   * In case the lock is currently owned by a different task, it will suspend the currently running task and
   * not allow it to run again until the lock is gained.
   * 
   * \param[in] task The task acquiring the lock
  */
  __USED__ inline void lock(HiCR::tasking::Task *task = HiCR::tasking::Task::getCurrentTask()) { lockBlockingImpl(task); }

  /**
   * Releases a lock currently owned by the currently running task.
   * 
   * \param[in] task The task releasing the lock
   * \note This function will produce an exception if trying to unlock a mutex not owned by the calling task. 
  */
  __USED__ inline void unlock(HiCR::tasking::Task *task = HiCR::tasking::Task::getCurrentTask())
  {
    if (ownsLock(task) == false) HICR_THROW_LOGIC("Trying to unlock a mutex that doesn't belong to this task");
    _lockValue = nullptr;
  }

  private:

  /**
   * Internal implementation of the blocking lock obtaining mechanism
   * 
   * \param[in] task The task acquiring the lock
  */
  __USED__ inline void lockBlockingImpl(HiCR::tasking::Task *task)
  {
    // Checking right away if we can get the lock
    bool success = lockNotBlockingImpl(task);

    // If not successful, then suspend the task
    if (success == false)
    {
      // Register a pending operation that will prevent task from being rescheduled until finished
      task->registerPendingOperation([&]() { return lockNotBlockingImpl(task); });

      // Prevent from re-executing task until the lock is obtained
      task->suspend();
    }
  }

  /**
   * Internal implementation of the non-blocking lock obtaining mechanism
   * 
   * \param[in] freeValue The expected value to see in the internal atomic lock value. The lock will fail and suspend the task if this value is not observed.
   * \param[in] task The desired value to assign to the lock value. If the expected value is observed, then this value is assigned atomically to the locl value.
   * \return True, if succeeded in acquiring the lock; false, otherwise.
  */
  __USED__ inline bool lockNotBlockingImpl(HiCR::tasking::Task *task, HiCR::tasking::Task *freeValue = nullptr) { return _lockValue.compare_exchange_weak(freeValue, task); }

  /**
   * Internal atomic value of the task-aware lock
  */
  std::atomic<HiCR::tasking::Task *> _lockValue = nullptr;
};

} // namespace tasking

} // namespace HiCR