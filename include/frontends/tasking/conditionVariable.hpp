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

#include <unordered_set>
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
   * Checks whether the given condition predicate evaluates to true.
   *  - If it does, then returns immediately.
   *  - If it does not, adds the task to the notification list and suspends it. The task will not be issued for execution again until
   *   * (a) The task is notified, and
   *   * (b) the condition is satisfied.
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
    if (keepWaiting == true)
    {
      // Insert oneself in the waiting task list
      _mutex.lock();
      _waitingTasks.insert(currentTask);
      _mutex.unlock();

      // Register a pending operation that will prevent task from being rescheduled until finished
      currentTask->registerPendingOperation([&]() {
        // Gaining lock by insisting until we get it
        while (_mutex.trylock(currentTask) == false)
          ;

        // Checking whether this task has been notified
        bool isNotified = _waitingTasks.contains(currentTask) == false;

        // Releasing lock
        _mutex.unlock(currentTask);

        // If not notified, re-add task to notification list and stop evaluating
        if (isNotified == false)
        {
          // Gaining lock by insisting until we get it
          while (_mutex.trylock(currentTask) == false)
            ;

          // Reinserting ourselves in the waiting task list
          _waitingTasks.insert(currentTask);

          // Releasing lock
          _mutex.unlock(currentTask);

          // Returning false because we haven't yet been notified
          return false;
        }

        // Gaining lock by insisting until we get it
        while (conditionMutex.trylock(currentTask) == false)
          ;

        // Checking actual condition
        bool isConditionSatisfied = conditionPredicate();

        // Releasing lock
        conditionMutex.unlock(currentTask);

        // Return whether the condition is satisfied
        return isConditionSatisfied;
      });

      // Suspending task
      currentTask->suspend();
    }
  }

  /**
   * Enables (notifies) one of the waiting tasks to check for the condition again.
   * 
   * \note No ordering is enforced. Any of the waiting tasks can potentially be notified regardless or arrival time.
  */
  void notifyOne()
  {
    _mutex.lock();
    if (_waitingTasks.empty() == false) _waitingTasks.erase(_waitingTasks.begin());
    _mutex.unlock();
  }

  /**
   * Enables (notifies) all of the waiting tasks to check for the condition again.
  */
  void notifyAll()
  {
    _mutex.lock();
    _waitingTasks.clear();
    _mutex.unlock();
  }

  private:

  /**
   * Internal mutex for accessing the waiting task set
  */
  tasking::Mutex _mutex;

  /**
   * A set of waiting tasks. No ordering is enforced here.
  */
  std::unordered_set<HiCR::tasking::Task *> _waitingTasks;
};

} // namespace tasking

} // namespace HiCR