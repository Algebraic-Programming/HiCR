/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file runtime.hpp
 * @brief This file implements the TaskR task class
 * @author Sergio Martin
 * @date 8/8/2023
 */

#pragma once

#include <hicr/task.hpp>
#include <taskr/common.hpp>
#include <vector>

namespace taskr
{

/**
 * Implementation of the TaskR task
 *
 * It holds the internal execution state of the task (implemented via a HiCR:Task), it's unique identifier (label) and its execution dependencies.
 */
class Task
{
  private:

  /**
   * HiCR Task object to implement TaskR tasks
   */
  HiCR::Task _hicrTask;

  /**
   * Tasks's label, chosen by the user
   */
  taskLabel_t _label;

  /**
   * Task execution dependency list. The task will be ready only if this container is empty.
   */
  std::vector<taskLabel_t> _taskDependencies;

  public:

  /**
   * Constructor for the TaskR task class. It requires a user-defined function to execute and a label.
   * The task is considered finished when the function runs to completion.
   *
   * \param[in] label A user-defined unique identifier for the task. It is required for dependency management
   * \param[in] fc A user-defined function to run
   */
  __USED__ inline Task(const taskLabel_t label, const callback_t &fc) : _hicrTask(HiCR::Task([fc](){ fc(); })), _label(label) {}

  /**
   * Returns the underlying HiCR task
   *
   * \return A pointer to the underlying HiCR task
   */
  __USED__ inline HiCR::Task *getHiCRTask() { return &_hicrTask; }

  /**
   * Returns the task's label
   *
   * \return The task's label
   */
  __USED__ inline taskLabel_t getLabel() const { return _label; }

  /**
   * Adds an execution depedency to this task. This means that this task will not be ready to execute until and unless
   * the task referenced by this label has finished executing.
   *
   * \param[in] task The label of the task upon whose completion this task should depend
   */
  __USED__ inline void addTaskDependency(const taskLabel_t task)
  {
    _taskDependencies.push_back(task);
  };

  /**
   * Returns Returns this task's dependency list.
   *
   * \return A constant reference to this task's dependencies vector.
   */
  __USED__ inline const std::vector<taskLabel_t> &getDependencies()
  {
    return _taskDependencies;
  }
}; // class Task

} // namespace taskr
