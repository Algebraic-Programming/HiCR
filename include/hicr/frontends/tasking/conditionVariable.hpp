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

#include <chrono>
#include <queue>
#include <hicr/core/concurrent/queue.hpp>
#include "mutex.hpp"
#include "task.hpp"

namespace HiCR
{

namespace tasking
{

/**
 * Implementation of a task-aware Condition Variable in HiCR.
*/
class ConditionVariable
{
  public:

  ConditionVariable()  = default;
  ~ConditionVariable() = default;

  /**
   * (1) Checks whether the given condition predicate evaluates to true.
   *  - If it does, then returns immediately.
   *  - If it does not, adds the task to the notification list and suspends it.
   *    - When resumed, the task will repeat step (1)
   * 
   * \note The suspension of the task will not block the running thread.
   * \param[in] conditionMutex The mutual exclusion mechanism to use to prevent two tasks from evaluating the condition predicate simultaneously
   * \param[in] conditionPredicate The function that returns a boolean true if the condition is satisfied; false, if not.
  */
  void wait(tasking::Mutex &conditionMutex, std::function<bool(void)> conditionPredicate)
  {
    auto currentTask = HiCR::tasking::Task::getCurrentTask();

    // Checking on the condition
    conditionMutex.lock();
    bool keepWaiting = conditionPredicate() == false;
    conditionMutex.unlock();

    // If the condition is not satisfied, suspend until we're notified and the condition is satisfied
    while (keepWaiting == true)
    {
      // Insert oneself in the waiting task list
      _mutex.lock();
      _waitingTasks.push(currentTask);
      _mutex.unlock();

      // Suspending task now
      currentTask->suspend();

      // After being notified, check on the condition again
      conditionMutex.lock();
      keepWaiting = conditionPredicate() == false;
      conditionMutex.unlock();
    }
  }

  /**
   * Checks whether the given condition predicate evaluates to true.
   *  - If it does, then returns immediately.
   *  - If it does not, adds the task to the notification list and suspends it.
   *    - When resumed, the task will check total wait time
   *       - If wait time is smaller than timeout, repeat step (1)
   *       - If wat time exceeds timeout, return immediately
   * 
   * \note The suspension of the task will not block the running thread.
   * \param[in] conditionMutex The mutual exclusion mechanism to use to prevent two tasks from evaluating the condition predicate simultaneously
   * \param[in] conditionPredicate The function that returns a boolean true if the condition is satisfied; false, if not.
   * \param[in] timeout The amount of microseconds provided as timeout 
   * 
   * \return True, if the task is returning before timeout; false, if otherwise.
  */
  bool waitFor(tasking::Mutex &conditionMutex, std::function<bool(void)> conditionPredicate, size_t timeout)
  {
    auto currentTask = HiCR::tasking::Task::getCurrentTask();

    // Checking on the condition
    conditionMutex.lock();
    bool keepWaiting = conditionPredicate() == false;
    conditionMutex.unlock();

    // If predicate is satisfied, return immediately
    if (keepWaiting == false) return true;

    // Taking current time
    auto startTime = std::chrono::high_resolution_clock::now();

    // If the condition is not satisfied, suspend until we're notified and the condition is satisfied
    while (keepWaiting == true)
    {
      // Insert oneself in the waiting task list
      _mutex.lock();
      _waitingTasks.push(currentTask);
      _mutex.unlock();

      // Suspending task now
      currentTask->suspend();

      // After being notified, check on timeout
      auto currentTime = std::chrono::high_resolution_clock::now();
      auto elapsedTime = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - startTime);
      if (elapsedTime > std::chrono::duration<size_t, std::micro>(timeout)) return false;

      // After being notified, check on the condition again
      conditionMutex.lock();
      keepWaiting = conditionPredicate() == false;
      conditionMutex.unlock();
    }

    // Return true if the exit was due to satisfied condition predicate
    return true;
  }

  /**
   * Suspends the tasks unconditionally, and resumes after notification
   *
   * \note The suspension of the task will not block the running thread.
   * 
   * \param[in] timeout The amount of microseconds provided as timeout 
   * \return True, if the task is returning before timeout; false, if otherwise.
  */
  bool waitFor(size_t timeout)
  {
    auto currentTask = HiCR::tasking::Task::getCurrentTask();

    // Insert oneself in the waiting task list
    _mutex.lock();
    _waitingTasks.push(currentTask);
    _mutex.unlock();

    // Taking current time
    auto startTime = std::chrono::high_resolution_clock::now();

    // Suspending task now
    currentTask->suspend();

    // After being notified, check on timeout
    auto currentTime = std::chrono::high_resolution_clock::now();
    auto elapsedTime = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - startTime);
    if (elapsedTime > std::chrono::duration<size_t, std::micro>(timeout)) return false;

    return true;
  }

  /**
   * Suspends the tasks unconditionally, and resumes after notification
   * 
   * \note The suspension of the task will not block the running thread.
  */
  void wait()
  {
    auto currentTask = HiCR::tasking::Task::getCurrentTask();

    // Insert oneself in the waiting task list
    _mutex.lock();
    _waitingTasks.push(currentTask);
    _mutex.unlock();

    // Suspending task now
    currentTask->suspend();
  }

  /**
   * Enables (notifies) one of the waiting tasks to check for the condition again.
   * 
   * \note No ordering is enforced. Any of the waiting tasks can potentially be notified regardless or arrival time.
  */
  void notifyOne()
  {
    _mutex.lock();

    // If there is a task waiting to be notified, do that now and take it out of the queue
    if (_waitingTasks.empty() == false)
    {
      _waitingTasks.front()->sendSyncSignal();
      _waitingTasks.pop();
    };

    // Releasing queue lock
    _mutex.unlock();
  }

  /**
   * Enables (notifies) all of the waiting tasks to check for the condition again.
  */
  void notifyAll()
  {
    _mutex.lock();

    // If there are tasks waiting to be notified, do that now and take them out of the queue
    while (_waitingTasks.empty() == false)
    {
      _waitingTasks.front()->sendSyncSignal();
      _waitingTasks.pop();
    };

    _mutex.unlock();
  }

  /**
   * Gets the number of tasks already waiting for a notification
   * 
   * @return The number of tasks already waiting for a notification
  */
  size_t getWaitingTaskCount() const { return _waitingTasks.size(); }

  private:

  /**
   * Internal mutex for accessing the waiting task set
  */
  tasking::Mutex _mutex;

  /**
   * A set of waiting tasks. No ordering is enforced here.
  */
  std::queue<HiCR::tasking::Task *> _waitingTasks;
};

} // namespace tasking

} // namespace HiCR
