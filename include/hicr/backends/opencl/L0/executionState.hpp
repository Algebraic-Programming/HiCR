/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
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
#include <hicr/core/L0/executionState.hpp>
#include <hicr/backends/opencl/L0/executionUnit.hpp>
#include <hicr/backends/opencl/L0/device.hpp>

namespace HiCR
{

namespace backend
{

namespace opencl
{

namespace L0
{

/**
 * This class represents the execution state of a kernel for the OpenCL backend.
 * Since kernels are not preemptible, it does not offer suspend/resume functionality.
 */
class ExecutionState final : public HiCR::L0::ExecutionState
{
  public:

  /**
   * Constructor for an opencl execution state
   *
   * \param executionUnit execution unit containing the kernel to execute
   */
  ExecutionState(const std::shared_ptr<HiCR::L0::ExecutionUnit> executionUnit)
    : HiCR::L0::ExecutionState(executionUnit)
  {
    // Getting up-casted pointer for the execution unit
    auto e = dynamic_pointer_cast<opencl::L0::ExecutionUnit>(executionUnit);

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

} // namespace L0

} // namespace opencl

} // namespace backend

} // namespace HiCR
