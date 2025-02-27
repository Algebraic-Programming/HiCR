/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file task.hpp
 * @brief This file implements an example task class
 * @author Sergio Martin
 * @date 16/8/2024
 */

#pragma once

#include <hicr/frontends/tasking/common.hpp>
#include <hicr/frontends/tasking/task.hpp>
#include <hicr/backends/pthreads/L1/computeManager.hpp>

/**
 * An example of how a custom task can be built on top of the basic task definition in HiCR
 */
class Task final : public HiCR::tasking::Task
{
  public:

  /**
   * Task label type
   */
  typedef uint64_t label_t;

  Task()  = delete;
  ~Task() = default;

  /**
   * Returns the task's label
   *
   * \return The task's label
   */
  __INLINE__ label_t getLabel() const { return _label; }

  /**
   * Adds an execution depedency to this task. This means that this task will not be ready to execute until and unless
   * the task referenced by this label has finished executing.
   *
   * \param[in] task The label of the task upon whose completion this task should depend
   */
  __INLINE__ void addTaskDependency(const label_t task) { _taskDependencies.push_back(task); };

  /**
   * Returns Returns this task's dependency list.
   *
   * \return A constant reference to this task's dependencies vector.
   */
  __INLINE__ const std::vector<label_t> &getDependencies() { return _taskDependencies; }

  /**
   * Constructor for the TaskR task class. It requires a user-defined function to execute
   * The task is considered finished when the function runs to completion.
   *
   * @param[in] label The unique label to assign to this task
   * @param[in] executionUnit Specifies the function/kernel to execute.
   */
  __INLINE__ Task(const label_t label, const HiCR::backend::pthreads::L0::ExecutionUnit::pthreadFc_t threadFunction)
    : HiCR::tasking::Task(HiCR::backend::pthreads::L1::ComputeManager::createExecutionUnit(threadFunction), nullptr),
      _label(label)
  {}

  private:

  /**
   * Tasks's label, chosen by the user
   */
  const label_t _label;

  /**
   * Task execution dependency list. The task will be ready only if this container is empty.
   */
  std::vector<label_t> _taskDependencies;

}; // class Task
