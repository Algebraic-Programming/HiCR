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
 * @brief This file implements the execution state class for the acl backend
 * @author L. Terracciano
 * @date 1/11/2023
 */

#pragma once

#include <acl/acl.h>
#include <hicr/core/exceptions.hpp>
#include <hicr/core/executionState.hpp>
#include <hicr/backends/acl/executionUnit.hpp>
#include <hicr/backends/acl/device.hpp>

namespace HiCR::backend::acl
{

/**
 * This class represents the execution state of a stream of kernel for the acl backend.
 * Since kernels are not preemptible, it does not offer suspend/resume functionality.
 */
class ExecutionState final : public HiCR::ExecutionState
{
  public:

  /**
   * Constructor for an acl execution state
   *
   * \param executionUnit execution unit containing the kernel to execute
   */
  ExecutionState(const std::shared_ptr<HiCR::ExecutionUnit> executionUnit)
    : HiCR::ExecutionState(executionUnit)
  {
    // Getting up-casted pointer for the execution unit
    auto e = dynamic_pointer_cast<acl::ExecutionUnit>(executionUnit);

    // Checking whether the execution unit passed is compatible with this backend
    if (e == NULL) HICR_THROW_LOGIC("The execution unit of type '%s' is not supported by this backend\n", executionUnit->getType());

    _executionUnit = e;
  }

  /**
   * Default destructor
  */
  ~ExecutionState() = default;

  /**
   * Set the acl stream
   * 
   * \param stream
  */
  __INLINE__ void setStream(const aclrtStream stream) { _stream = stream; }

  /**
   * Synchronize and destroy the currently used stream
   */
  __INLINE__ void finalizeStream()
  {
    if (_isStreamActive)
    {
      // synchronize on the stream
      aclError err = aclrtSynchronizeStream(_stream);
      if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Failed to synchronize stream after kernel execution. Error %d", err);

      // Destroy the related event
      err = aclrtDestroyEvent(_syncEvent);
      if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Failed to destroy event. Error %d", err);

      // avoid deleting the stream more than once
      _isStreamActive = false;
    }
  }

  protected:

  /**
   * Internal implementation of resume routine.
   */
  __INLINE__ void resumeImpl() override
  {
    // Create an ACL event
    aclError err = aclrtCreateEvent(&_syncEvent);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not create synchronize event. Error %d", err);

    // Signal that the execution unit is running
    _isStreamActive = true;

    // start the sequence of kernels execution
    _executionUnit->start(_stream);

    // add an event at the end of the operations to query its status and check for completion
    err = aclrtRecordEvent(_syncEvent, _stream);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("can not set sync bit to 1. Error %d", err);
  }

  __INLINE__ void suspendImpl() { HICR_THROW_RUNTIME("Suspend functionality not supported by acl backend"); }

  /**
   * Internal implementation of checkFinalization routine. It periodically query the ACL event on the stream to check for completion and
   * automatically deletes the stream once it completes.
   *
   * \return whether all the kernels described in the execution unit finished.
   */
  __INLINE__ bool checkFinalizationImpl() override
  {
    // Check if the event has been processed
    aclrtEventRecordedStatus status;
    aclError                 err = aclrtQueryEventStatus(_syncEvent, &status);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("failed to query event status. err %d", err);

    // check the synchronization event status for stream completion
    if (status == ACL_EVENT_RECORDED_STATUS_NOT_READY) return false;

    // synchronize the stream and destroy it
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
  aclrtEvent _syncEvent;

  /**
   * Stream on which the execution unit kernels are scheduled.
   */
  aclrtStream _stream;

  /**
   * Keep track of the stream status
   */
  bool _isStreamActive = false;
};

} // namespace HiCR::backend::acl
