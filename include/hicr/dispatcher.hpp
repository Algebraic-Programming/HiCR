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
#include <hicr/common/definitions.hpp>
#include <hicr/task.hpp>
#include <vector>

namespace HiCR
{

class Worker;

/**
 * Defines a standard type for a pull function.
 */
typedef std::function<Task *(HiCR::Worker *)> pullFunction_t;

/**
 * Class definition for a pull-only task dispatcher object.
 *
 * This dispatcher delivers pending Tasks for execution by request and on real-time via a pull mechanism:
 *
 * When the pull function is called by a consumer, the dispatcher will execute a callback function defined by a producer which may return a Task on real-time.
 *
 */
class Dispatcher
{
  private:
  /**
   * Storage for the pull function, as defined by a task producer
   */
  pullFunction_t _pullFc;

  public:
  /**
   * Constructor for the task dispatcher
   *
   * \param[in] pullFc The function to call for obraining (pulling) new tasks. Should return NULL if no tasks are to be executed.
   */
  HICR_API Dispatcher(const pullFunction_t pullFc) : _pullFc(pullFc){};
  ~Dispatcher() = default;

  /**
   * Returns a task for execution, by calling the dispatcher's pull function callback, if defined.
   *
   * It will produce an exception if no pull function was defined.
   *
   * \param[in] worker A pointer to the calling worker. This is required for worker management (e.g., suspend/resume) at the upper layer.
   * \return Returns the pointer of a Task, as given by the pull function callback. If the callback returns no tasks for execution, then this function returns a NULL pointer.
   */
  HICR_API inline Task *pull(Worker *worker)
  {
    return _pullFc(worker);
  }
};

} // namespace HiCR
