/*
 *   Copyright 2025 Huawei Technologies Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file executionState.hpp
 * @brief nOS-V execution state class. Main job is to store the nosv task and its metadata
 * @author N. Baumann
 * @date 24/02/2025
 */
#pragma once

#include <memory>
#include <nosv.h>
#include <nosv/affinity.h>
#include <hicr/core/definitions.hpp>
#include <hicr/core/exceptions.hpp>
#include <hicr/core/executionState.hpp>

#include <hicr/backends/nosv/executionUnit.hpp>
#include <hicr/backends/nosv/common.hpp>

#ifdef ENABLE_INSTRUMENTATION
  #include <tracr.hpp>
#endif

namespace HiCR::backend::nosv
{

/**
 * This class is an abstract representation of the lifetime of an execution unit.
 * It exposes initialization, suspension and resume functionalities that should (ideally) be implemented for all execution/processing unit combinations.
 */
class ExecutionState final : public HiCR::ExecutionState
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
  __INLINE__ ExecutionState(const std::shared_ptr<HiCR::ExecutionUnit> &executionUnit, void *const argument = nullptr)
    : HiCR::ExecutionState(executionUnit)
  {
    // Getting up-casted pointer of the execution unit
    auto c = static_pointer_cast<HiCR::backend::nosv::ExecutionUnit>(executionUnit);

    // Checking whether the execution unit passed is compatible with this backend
    if (c == nullptr) HICR_THROW_LOGIC("The passed execution unit is not supported by this execution state type\n");
    
    // Initialize the nosv type with the new defined task type and its metadata
    check(nosv_type_init(&_executionStateTaskType, run_callback, NULL, completed_callback, "executionUnitTaskType", NULL, NULL, NOSV_TYPE_INIT_NONE));

    // nosv create the execution task
    check(nosv_create(&_executionStateTask, _executionStateTaskType, sizeof(taskMetadata_t), NOSV_CREATE_NONE));

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

// TraCR set trace of thread polling again (as it suspended his task)
#ifdef ENABLE_INSTRUMENTATION
    INSTRUMENTATION_THREAD_MARK_SET((long)2);
#endif

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
   * nOS-V runtime callback wrapper for the fc
   */
  static __INLINE__ void run_callback(nosv_task_t task) {
// TraCR set trace of thread executing a task
#ifdef ENABLE_INSTRUMENTATION
  INSTRUMENTATION_THREAD_MARK_SET((long)0);
#endif
    
    // Accessing metadata from the task
    auto metadata = (taskMetadata_t *)getTaskMetadata(task);
    
    // Unblocking the worker mainLoop  as the run_callback successfully has been called from here
    if (metadata->mainLoop) check(nosv_barrier_wait(metadata->mainLoop_barrier));

    // Get the fc
    auto fc = metadata->fc;

    // Get the argument pointer to pass over the function
    auto arg = metadata->arg;

    // Executing the function (Else we throw at runtime)
    if (fc) { fc(arg); }
    else { HICR_THROW_RUNTIME("Error: No valid callback function.\n"); }
  }

  /**
   * The completed_callback function of nosv. 
   * This will be called after the run_callback and it is save to continue the parent_task
   */
  static __INLINE__ void completed_callback(nosv_task_t task)
  {
    auto metadata = (taskMetadata_t *)getTaskMetadata(task);

    // mark task as completed
    metadata->executionState->_completed = true;

    // Resume the parent task as its child task has just finished
    if (!(metadata->mainLoop))
    {
      if (!(metadata->parent_task)) HICR_THROW_RUNTIME("The parent task is not existing (i.e. NULL).");
      
      // sleeping somehow helps the problem of this rare bug not occuring:
      // [HiCR] Runtime Exception: Task has to be either in suspended or in finished state but I got State: 2. IsFinished: 0
      // sleep(0.01);
      check(nosv_submit(metadata->parent_task, NOSV_SUBMIT_UNLOCKED));
    }

    // Destroying this task
    // TODO: Destroy a nOS-V task when no longer needed. Destroying the nOS-V task here does not work for all the TaskR examples (e.g. abcTasks and Jacobi3D).
    // check(nosv_destroy(task, NOSV_DESTROY_NONE));
  }

  /**
   * nosv task type of the executionUnit
   */
  nosv_task_type_t _executionStateTaskType;

  /**
   * boolean to determine if the fc has finished.
   */
  volatile bool _completed = false;
};

} // namespace HiCR::backend::nosv
