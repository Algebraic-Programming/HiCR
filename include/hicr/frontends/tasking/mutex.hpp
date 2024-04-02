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

#include <mutex>
#include <queue>
#include <hicr/core/exceptions.hpp>
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
  __INLINE__ bool ownsLock(HiCR::tasking::Task *task = HiCR::tasking::Task::getCurrentTask()) { return _ownerTask == task; }

  /**
   * Tries to obtain the lock and returns immediately if it fails.
   *
   * \param[in] task The task acquiring the lock
   * @return True, if it succeeded in obtaining the lock; false, otherwise.
  */
  __INLINE__ bool trylock(HiCR::tasking::Task *task = HiCR::tasking::Task::getCurrentTask())
  {
    _mutex.lock();
    bool success = lockNotBlockingImpl(task);
    _mutex.unlock();
    return success;
  }

  /**
   * Obtains the mutual exclusion lock.
   * 
   * In case the lock is currently owned by a different task, it will suspend the currently running task and
   * not allow it to run again until the lock is gained.
   * 
   * \param[in] task The task acquiring the lock
  */
  __INLINE__ void lock(HiCR::tasking::Task *task = HiCR::tasking::Task::getCurrentTask()) { lockBlockingImpl(task); }

  /**
   * Releases a lock currently owned by the currently running task.
   * 
   * \param[in] task The task releasing the lock
   * \note This function will produce an exception if trying to unlock a mutex not owned by the calling task. 
  */
  __INLINE__ void unlock(HiCR::tasking::Task *task = HiCR::tasking::Task::getCurrentTask())
  {
    if (ownsLock(task) == false) HICR_THROW_LOGIC("Trying to unlock a mutex that doesn't belong to this task");

    _mutex.lock();

    _ownerTask = nullptr;
    if (_queue.empty() == false)
    {
      _ownerTask = _queue.front();
      _queue.pop();
      _ownerTask->sendSyncSignal();
    }

    _mutex.unlock();
  }

  private:

  /**
   * Internal implementation of the blocking lock obtaining mechanism
   * 
   * \param[in] task The task acquiring the lock
  */
  __INLINE__ void lockBlockingImpl(HiCR::tasking::Task *task)
  {
    _mutex.lock();

    bool isLockFree = lockNotBlockingImpl(task);

    // If not successful, then insert task in the pending queue and suspend it
    if (isLockFree == false)
    {
      // Adding itself to the queue
      _queue.push(task);

      // Releasing lock
      _mutex.unlock();

      // Prevent from re-executing task until the lock is obtained
      task->suspend();

      // return now
      return;
    }

    _mutex.unlock();
  }

  /**
   * Internal implementation of the non-blocking lock obtaining mechanism
   * 
   * \param[in] task The desired value to assign to the lock value. If the expected value is observed, then this value is assigned atomically to the locl value.
   * \return True, if succeeded in acquiring the lock; false, otherwise.
  */
  __INLINE__ bool lockNotBlockingImpl(HiCR::tasking::Task *task)
  {
    bool isLockFree = _ownerTask == nullptr;
    if (isLockFree) _ownerTask = task;
    return isLockFree;
  }

  /**
   * Internal mutex for protecting the internal state and the task queue
  */
  std::mutex _mutex;

  /**
   * Internal state
  */
  HiCR::tasking::Task *_ownerTask = nullptr;

  /**
   * Pending task queue
  */
  std::queue<HiCR::tasking::Task *> _queue;
};

} // namespace tasking

} // namespace HiCR