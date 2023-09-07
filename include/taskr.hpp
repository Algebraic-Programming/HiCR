/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file taskr.hpp
 * @brief Main interface for the TaskR frontend
 * @author Sergio Martin
 * @date 8/8/2023
 */

#pragma once
#include <taskr/runtime.hpp>
#include <taskr/task.hpp>

namespace taskr
{

/**
 * Boolean indicating whether the runtime system was initialized
 */
inline bool _runtimeInitialized = false;

/**
 * Standalone function used to add an already created task to the TaskR runtime task queue
 * This function can be called at any point before or during the execution of the TaskR runtime.
 *
 * \param[in] task Task to add
 */
__USED__ inline void addTask(Task *task)
{
  if (_runtimeInitialized == false)
    LOG_ERROR("Attempting to use Taskr without first initializing it.");

  // Adding task
  _runtime->addTask(task);
}

/**
 * Standalone function to instantiate and initialize the TaskR runtime singleton.
 *
 * \param[in] backend HiCR backend from whence to source compute resources to run the workers
 */
__USED__ inline void initialize(HiCR::Backend *backend)
{
  // Instantiating singleton
  _runtime = new Runtime(backend);

  // Set initialized state
  _runtimeInitialized = true;
}

/**
 * Standalone function to define the maximum amount of running workers in the TaskR runtime.
 *
 * By defining a maximum, it is possible to decrease the amount of CPU cores being, even if more workers have been created.
 * It is useful for situations where only a few tasks must run and one would like to save energy.
 *
 * \param[in] max Maximum number of workers to keep in a running state
 */
__USED__ inline void setMaximumActiveWorkers(const ssize_t max)
{
  // Storing new maximum active worker count
  _runtime->setMaximumActiveWorkers(max);
}

/**
 * Standalone function to start the execution of the TaskR runtime
 *
 * Upon start, TaskR gains control of the current context and executes any added tasks, based on their dependencies, until no tasks remain.
 *
 * \param[in] computeResourceList The list of compute resources, provided by the specified backend, to use in creating the processing units to assign to workers to execute tasks. If this value is not provided, TaskR will allocate as many processing units as compute resources detected by the backend.
 */
__USED__ inline void run(const HiCR::computeResourceList_t &computeResourceList = HiCR::computeResourceList_t())
{
  if (_runtimeInitialized == false)
    LOG_ERROR("Attempting to use Taskr without first initializing it.");

  // Running Taskr
  _runtime->run(computeResourceList);
}

/**
 * Standalone function to free up any remaining memory allocated to run TaskR.
 *
 * Should not be called while TaskR is running.
 */
__USED__ inline void finalize()
{
  if (_runtimeInitialized == false)
    LOG_ERROR("Attempting to use Taskr without first initializing it.");

  // Freeing up singleton
  delete _runtime;

  // Setting runtime to non-initialized
  _runtimeInitialized = false;
}

} // namespace taskr
