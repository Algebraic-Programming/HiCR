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
 * @brief This file implements the execution state class for the OpenCL backend
 * @author L. Terracciano
 * @date 07/03/2025
 */

#pragma once

#include <CL/opencl.h>
#include <hicr/core/exceptions.hpp>
#include <hicr/core/executionState.hpp>
#include <hicr/backends/opencl/executionUnit.hpp>
#include <hicr/backends/opencl/device.hpp>

namespace HiCR::backend::opencl
{
/**
 * This class represents the execution state of a kernel for the OpenCL backend.
 * Since kernels are not preemptible, it does not offer suspend/resume functionality.
 */
class ExecutionState final : public HiCR::ExecutionState
{
  public:

  /**
   * Constructor for an opencl execution state
   *
   * \param executionUnit execution unit containing the kernel to execute
   */
  ExecutionState(const std::shared_ptr<HiCR::ExecutionUnit> executionUnit)
    : HiCR::ExecutionState(executionUnit)
  {
    // Getting up-casted pointer for the execution unit
    auto e = dynamic_pointer_cast<opencl::ExecutionUnit>(executionUnit);

    // Checking whether the execution unit passed is compatible with this backend
    if (e == NULL) HICR_THROW_LOGIC("The execution unit of type '%s' is not supported by this backend\n", executionUnit->getType());

    _executionUnit = e;
  }

  ~ExecutionState() = default;

  /**
   * Set the OpenCL queue
   * 
   * \param[in] queue
  */
  __INLINE__ void setQueue(cl::CommandQueue *queue) { _queue = queue; }

  /**
   * Synchronize and destroy the currently used queue
   */
  __INLINE__ void finalizeStream()
  {
    if (_isStreamActive == true)
    {
      // synchronize on the queue
      auto err = _syncEvent.wait();
      if (err != CL_SUCCESS) [[unlikely]] { HICR_THROW_RUNTIME("Failed to wait after kernel execution. Error %d", err); }

      // avoid deleting the queue more than once
      _isStreamActive = false;
    }
  }

  protected:

  /**
   * Internal implementation of resume routine.
   */
  __INLINE__ void resumeImpl() override
  {
    // Create event to wait for completion and check kernel status
    _syncEvent = cl::Event();

    _isStreamActive = true;

    // start the sequence of kernels execution
    _executionUnit->start(_queue);

    // add an event at the end of the operations to query its status and check for completion
    auto err = _queue->enqueueMarkerWithWaitList(nullptr, &_syncEvent);
    if (err != CL_SUCCESS) [[unlikely]] { HICR_THROW_RUNTIME("Failed to write event in the queue", err); }
  }

  __INLINE__ void suspendImpl() { HICR_THROW_RUNTIME("Suspend functionality not supported by OpenCL backend"); }

  /**
   * Internal implementation of checkFinalization routine. It periodically query the OpenCL event on the queue to check for completion and
   * automatically deletes the queue once it completes.
   *
   * \return whether all the kernels described in the execution unit finished.
   */
  __INLINE__ bool checkFinalizationImpl() override
  {
    // Check if the event has been processed
    auto status = _syncEvent.getInfo<CL_EVENT_COMMAND_EXECUTION_STATUS>();

    if (status != CL_COMPLETE) { return false; }

    // synchronize the queue and destroy it
    finalizeStream();

    return true;
  }

  private:

  /**
   * Execution unit containing the kernel operations to execute
   */
  std::shared_ptr<ExecutionUnit> _executionUnit;

  /**
   * Synchronization event to check for stream completion
   */
  cl::Event _syncEvent;

  /**
   * OpenCL command queue
   */
  cl::CommandQueue *_queue;

  /**
   * Keep track of the stream status
   */
  bool _isStreamActive = false;
};

} // namespace HiCR::backend::opencl
