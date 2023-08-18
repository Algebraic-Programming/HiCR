/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file dispatcher.hpp
 * @brief Provides a class definition for the task dispatcher object.
 * @author S. M. Martin
 * @date 13/7/2023
 */

#pragma once

#include <hicr/common/concurrentQueue.hpp>
#include <hicr/task.hpp>
#include <vector>

namespace HiCR
{

/**
 * Defines a standard type for a pull function.
 */
typedef std::function<Task *(void)> pullFunction_t;

/**
 * Class definition for a generic multi-consumer multi-producer task dispatcher object.
 *
 * This dispatcher delivers pending Tasks for execution by request and on real-time via two mechanisms:
 *
 * - Push and Pop: Tasks are pushed by a producer into the dispatcher's internal FIFO storage and later popped by a consumer.
 * - Pull: When the pull function is called by a consumer, the dispatcher will execute a callback function defined by a producer which may return a Task on real-time.
 *
 * For performance, the combined PopOrPull function is provided, whereas a consumer can check both mechanisms described above for pending tasks.
 *
 */
class Dispatcher
{
  private:
  /**
   * Storage for the pull function, as defined by a task producer
   */
  pullFunction_t _pullFc;

  /**
   * Boolean to register whether a pull function was defined. Initially set as false.
   */
  bool isPullFunctionDefined = false;

  /**
   * Internal storage for pushed tasks
   */
  common::ConcurrentQueue<Task *, MAX_QUEUED_TASKS> _queue;

  public:
  Dispatcher() = default;
  ~Dispatcher() = default;

  /**
   * This function sets the callback function for the pull operation. It will be called whenever a consumer runs the \a pull function.
   *
   * \param[in] pullFc The callback function for pulling tasks.
   *
   * \internal The function is stored by copy, to prevent false accesses on locally created lambda functions.
   */
  inline void setPullFunction(const pullFunction_t pullFc)
  {
    _pullFc = pullFc;
    isPullFunctionDefined = true;
  }

  /**
   * Removes the currently defined pull function.
   *
   * \internal In its implementation, we only clear the associated boolean, as there is no need to replace the function data itself.
   */
  inline void clearPullFunction()
  {
    isPullFunctionDefined = false;
  }

  /**
   * Returns a task for execution, by calling the dispatcher's pull function callback, if defined.
   *
   * It will produce an exception if no pull function was defined.
   *
   * \return Returns the pointer of a Task, as given by the pull function callback. If the callback returns no tasks for execution, then this function returns a NULL pointer.
   */
  inline Task *pull()
  {
    if (isPullFunctionDefined == false) LOG_ERROR("Trying to pull on dispatcher but the pull function is not defined\n");
    return _pullFc();
  }

  /**
   * Inserts a new Task for execution in the dispatcher's internal FIFO storage.
   *
   * \param[in] task A pointer to the task to store.
   */
  inline void push(Task *task)
  {
    _queue.push(task);
  }

  /**
   * Obtains the earliest pushed task in the internal FIFO storage and removes it from the storage.
   *
   * \return The task to execute, NULL pointer if the internal storage is currently empty.
   */
  inline Task *pop()
  {
    return _queue.pop();
  }

  /**
   * Attempts first a \a pop operation and then, if it returns a NULL pointer, runs the #pull function.
   *
   * It will produce an exception if no pull function was defined.
   *
   * \return The task to execute, NULL pointer if the internal storage is currently empty and the pull function also returns a NULL pointer.
   */
  inline Task *pullOrPop()
  {
    Task *task = pop();

    if (isPullFunctionDefined == true && task == NULL) task = pull();

    return task;
  }
};

} // namespace HiCR
