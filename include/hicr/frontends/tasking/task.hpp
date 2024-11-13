/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file deployer.hpp
 * @brief This file implements the HiCR task class
 * @author Sergio Martin
 * @date 8/8/2023
 */

#pragma once

#include <atomic>
#include <memory>
#include <queue>
#include <utility>
#include <vector>
#include <hicr/core/definitions.hpp>
#include <hicr/core/exceptions.hpp>
#include <hicr/core/L0/executionState.hpp>
#include <hicr/core/L0/executionUnit.hpp>
#include <hicr/core/L0/processingUnit.hpp>
#include "callbackMap.hpp"
#include "common.hpp"

namespace HiCR::tasking
{

class Worker;

/**
 * This class defines the basic execution unit managed by TaskR.
 *
 * It includes a function to execute, an internal state, and an callback map that triggers callbacks (if defined) whenever a state transition occurs.
 *
 * The function represents the entire lifetime of the task. That is, a task executes a single function, the one provided by the user, and will reach a terminated state after the function is fully executed.
 *
 * A task may be suspended before the function is fully executed. This is either by voluntary yielding, or by reaching an synchronous operation that prompts it to suspend. These two suspension reasons will result in different states.
 */
class Task
{
  public:

  /**
   * Enumeration of possible task-related callbacks that can trigger a user-defined function callback
   */
  enum callback_t
  {
    /**
     * Triggered as the task starts or resumes execution
     */
    onTaskExecute,

    /**
     * Triggered as the task is preempted into suspension by an asynchronous callback
     */
    onTaskSuspend,

    /**
     * Triggered as the task finishes execution
     */
    onTaskFinish,
  };

  /**
   * Type definition for the task's callback map
   */
  using taskCallbackMap_t = HiCR::tasking::CallbackMap<Task *, callback_t>;

  Task()  = delete;
  ~Task() = default;

  /**
   * Constructor for the TaskR task class. It requires a user-defined function to execute
   * The task is considered finished when the function runs to completion.
   *
   * @param[in] executionUnit Specifies the function/kernel to execute.
   * @param[in] callbackMap Pointer to the callback map callbacks to be called by the task
   */
  __INLINE__ Task(std::shared_ptr<HiCR::L0::ExecutionUnit> executionUnit, taskCallbackMap_t *callbackMap = nullptr)
    : _executionUnit(std::move(executionUnit)),
      _callbackMap(callbackMap){};

  /**
   * Sets the task's callback map. This map will be queried whenever a state transition occurs, and if the map defines a callback for it, it will be executed.
   *
   * @param[in] callbackMap A pointer to an callback map
   */
  __INLINE__ void setCallbackMap(taskCallbackMap_t *callbackMap) { _callbackMap = callbackMap; }

  /**
   * Gets the task's callback map.
   *
   * @return A pointer to the task's an callback map. NULL, if not defined.
   */
  __INLINE__ taskCallbackMap_t *getCallbackMap() { return _callbackMap; }

  /**
   * Queries the task's internal state.
   *
   * @return The task internal state
   *
   * \internal This is not a thread safe operation.
   */
  __INLINE__ const HiCR::L0::ExecutionState::state_t getState()
  {
    // If the execution state has not been initialized then return the value expliclitly
    if (_executionState == nullptr) return HiCR::L0::ExecutionState::state_t::uninitialized;

    // Otherwise just query the initial execution state
    return _executionState->getState();
  }

  /**
   * Sets the execution unit assigned to this task
   *
   * \param[in] executionUnit The execution unit to assign to this task
   */
  __INLINE__ void setExecutionUnit(std::shared_ptr<HiCR::L0::ExecutionUnit> executionUnit) { _executionUnit = std::move(executionUnit); }

  /**
   * Returns the execution unit assigned to this task
   *
   * \return The execution unit assigned to this task
   */
  [[nodiscard]] __INLINE__ std::shared_ptr<HiCR::L0::ExecutionUnit> getExecutionUnit() const { return _executionUnit; }

  /**
   * Implements the initialization routine of a task, that stores and initializes the execution state to run to completion
   *
   * \param[in] executionState A previously initialized execution state
   */
  __INLINE__ void initialize(std::unique_ptr<HiCR::L0::ExecutionState> executionState)
  {
    if (getState() != HiCR::L0::ExecutionState::state_t::uninitialized)
      HICR_THROW_LOGIC("Attempting to initialize a task that has already been initialized (State: %d).\n", getState());

    // Getting execution state as a unique pointer (to prcallback sharing the same state among different tasks)
    _executionState = std::move(executionState);
  }

  /**
   * This function starts running a task. It needs to be performed by a worker, by passing a pointer to itself.
   *
   * The execution of the task will trigger change of state from initialized to running. Before reaching the terminated state, the task might transition to some of the suspended states.
   */
  __INLINE__ void run()
  {
    if (getState() != HiCR::L0::ExecutionState::state_t::initialized && getState() != HiCR::L0::ExecutionState::state_t::suspended)
      HICR_THROW_RUNTIME("Attempting to run a task that is not in a initialized or suspended state (State: %d).\n", getState());

    // Triggering execution callback, if defined
    if (_callbackMap != nullptr) _callbackMap->trigger(this, callback_t::onTaskExecute);

    // Now resuming the task's execution
    _executionState->resume();

    // Checking execution state finalization
    _executionState->checkFinalization();

    // Getting state after execution
    const auto state = getState();

    // If the task is suspended and callback map is defined, trigger the corresponding callback.
    if (state == HiCR::L0::ExecutionState::state_t::suspended)
      if (_callbackMap != nullptr) _callbackMap->trigger(this, callback_t::onTaskSuspend);

    // If the task is still running (no suspension), then the task has fully finished executing. If so,
    // trigger the corresponding callback, if the callback map is defined. It is important that this function
    // is called from outside the context of a task to allow the upper layer to free its memory upon finishing
    if (state == HiCR::L0::ExecutionState::state_t::finished)
      if (_callbackMap != nullptr) _callbackMap->trigger(this, callback_t::onTaskFinish);
  }

  /**
   * This function yields the execution of the task, and returns to the worker's context.
   */
  __INLINE__ void suspend()
  {
    if (getState() != HiCR::L0::ExecutionState::state_t::running) HICR_THROW_RUNTIME("Attempting to yield a task that is not in a running state (State: %d).\n", getState());

    // Yielding execution back to worker
    _executionState->suspend();
  }

  private:

  /**
   * Execution unit that will be instantiated and executed by this task
   */
  std::shared_ptr<HiCR::L0::ExecutionUnit> _executionUnit;

  /**
   *  Map of callbacks to trigger
   */
  taskCallbackMap_t *_callbackMap = nullptr;

  /**
   * Internal execution state of the task. Will change based on runtime scheduling callbacks
   */
  std::unique_ptr<HiCR::L0::ExecutionState> _executionState = nullptr;

}; // class Task

} // namespace HiCR::tasking
