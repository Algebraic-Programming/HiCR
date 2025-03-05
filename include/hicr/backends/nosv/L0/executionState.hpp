/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file executionState.hpp
 * @brief nOS-V execution state class. Main job is to store the nosv task and its metadata
 * @author N.Baumann
 * @date 24/02/2025
 */
#pragma once

#include <memory>
#include <nosv.h>
#include <nosv/affinity.h>
#include <hicr/core/definitions.hpp>
#include <hicr/core/exceptions.hpp>
#include <hicr/core/L0/executionState.hpp>

#include <hicr/backends/nosv/L0/executionUnit.hpp>
#include <hicr/backends/nosv/common.hpp>

namespace HiCR::backend::nosv::L0
{

/**
 * This class is an abstract representation of the lifetime of an execution unit.
 * It exposes initialization, suspension and resume functionalities that should (ideally) be implemented for all execution/processing unit combinations.
 */
class ExecutionState final : public HiCR::L0::ExecutionState
{
  public:

  /**
   * nosv MetaData place holder for the execution state
   */
  struct taskMetadata_t
  {
    /**
     * Boolean to check if this is the worker mainLoop task
     */
    volatile bool mainLoop = false;

    /**
     * nosv barrier for the worker mainLoop task
     * This is need as the submitted task from the worker mainLoop has to wait until the run_callback successfully executed
     */
    nosv_barrier_t mainLoop_barrier;

    /**
     * the passed function
     */
    std::function<void(void *)> fc = nullptr;

    /**
     * the function arguments
     */
    void *arg = nullptr;

    /**
     * The parent task from which this task has been submitted
     */
    nosv_task_t parent_task = NULL;

    /**
     * A pointer to the execution state class corresponding to this task
     */
    ExecutionState *executionState = nullptr;
  };

  /**
   * nosv task for the executionState
   */
  nosv_task_t _executionStateTask;

  /**
   * To save memory, the initialization of execution states (i.e., allocation of required structures) is deferred until this function is called.
   *
   * \param[in] executionUnit Represents a replicable executable unit (e.g., function, kernel) to execute
   * \param[in] argument Argument (closure) to pass to the function to be ran
   */
  __INLINE__ ExecutionState(const std::shared_ptr<HiCR::L0::ExecutionUnit> &executionUnit, void *const argument = nullptr)
    : HiCR::L0::ExecutionState(executionUnit)
  {
    // Getting up-casted pointer of the execution unit
    auto c = static_pointer_cast<HiCR::backend::nosv::L0::ExecutionUnit>(executionUnit);

    // Checking whether the execution unit passed is compatible with this backend
    if (c == nullptr) HICR_THROW_LOGIC("The passed execution unit is not supported by this execution state type\n");

    // nOS-V runtime callback wrapper for the fc
    nosv_task_run_callback_t run_callback = [](nosv_task_t task) {
      // Accessing metadata from the task
      auto TaskMetadata = (taskMetadata_t *)getTaskMetadata(task);

      // Unblocking the worker mainLoop  as the run_callback successfully has been called from here
      if (TaskMetadata->mainLoop) check(nosv_barrier_wait(TaskMetadata->mainLoop_barrier));

      // Get the fc
      auto fc = TaskMetadata->fc;

      // Get the argument pointer to pass over the function
      auto arg = TaskMetadata->arg;

      // Executing the function (Else we throw at runtime)
      if (fc) { fc(arg); }
      else { HICR_THROW_RUNTIME("Error: No valid callback function.\n"); }
    };

    // Initialize the nosv type with the new defined task type and its metadata
    check(nosv_type_init(&_executionUnitTaskType, run_callback, NULL, completed_callback, "executionUnitTaskType", NULL, NULL, NOSV_TYPE_INIT_NONE));

    // nosv create the execution task
    check(nosv_create(&_executionStateTask, _executionUnitTaskType, sizeof(taskMetadata_t), NOSV_CREATE_NONE));

    // Access the execution state task metadata
    auto metadata = (taskMetadata_t *)getTaskMetadata(_executionStateTask);

    // Store the function and function argument in the metadata
    metadata->executionState = this;
    metadata->fc             = c->getFunction();
    metadata->arg            = argument;
  }

  protected:

  /**
   * Backend-specific implementation of the resume function
   */
  __INLINE__ void resumeImpl() override
  {
    // Get current self task
    nosv_task_t current_task = nosv_self();

    // Access this execution state task metadata
    auto metadata = (taskMetadata_t *)getTaskMetadata(_executionStateTask);

    // Store the parent task in this task
    metadata->parent_task = current_task;

    // Set the affinity of this task to be the same as the current task.
    nosv_affinity_t self_affinity = nosv_get_task_affinity(current_task);
    nosv_set_task_affinity(_executionStateTask, &self_affinity);

    // submit (i.e. execute) the execution state task
    check(nosv_submit(_executionStateTask, NOSV_SUBMIT_NONE));

    // Pause this current task until either _executionStateTask suspends or finalized
    check(nosv_pause(NOSV_PAUSE_NONE));
  }

  /**
   * Backend-specific implementation of the suspend function
   */
  void suspendImpl() override
  {
    // Get current self task
    nosv_task_t self_task = nosv_self();

    // Check if the self task is actually this execution state task (otherwise is illegal as only the self task is to suspend itself)
    if (self_task != _executionStateTask) HICR_THROW_RUNTIME("Those should be the same task. nosv_self(): %p, _executionStateTask: %p", &self_task, &_executionStateTask);

    auto metadata = (taskMetadata_t *)getTaskMetadata(self_task);

    // Resume the parent task to continue running other tasks.
    check(nosv_submit(metadata->parent_task, NOSV_SUBMIT_NONE));

    // Now suspending this execution state.
    check(nosv_pause(NOSV_PAUSE_NONE));
  }

  /**
   * Backend-specific implementation of the checkFinalization function
   *
   * \return True, if the execution has finalized; False, otherwise.
   */
  bool checkFinalizationImpl() override { return _completed; }

  private:

  /**
   * The completed_callback function of nosv. 
   * This will be called after the run_callback and it is save to continue the parent_task
   */
  static __INLINE__ void completed_callback(nosv_task_t task)
  {
    auto metadata = (taskMetadata_t *)getTaskMetadata(task);

    metadata->executionState->_completed = true;

    // Resume the parent task as its child task has just finished
    if (!(metadata->mainLoop))
    {
      if (!(metadata->parent_task)) HICR_THROW_RUNTIME("The parent task is not existing (i.e. NULL).");
      check(nosv_submit(metadata->parent_task, NOSV_SUBMIT_UNLOCKED));
    }

    // destroying this task
    check(nosv_destroy(task, NOSV_DESTROY_NONE));
  }

  /**
   * nosv task type of the executionUnit
   */
  nosv_task_type_t _executionUnitTaskType;

  /**
   * boolean to determine if the fc has finished.
   */
  volatile bool _completed = false;
};

} // namespace HiCR::backend::nosv::L0
