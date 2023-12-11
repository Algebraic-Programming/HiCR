/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file executionState.hpp
 * @brief This file implements the execution state class for the ascend backend
 * @author S. M. Martin & L. Terracciano
 * @date 1/11/2023
 */

#pragma once

#include <acl/acl.h>
#include <hicr/common/exceptions.hpp>
#include <hicr/L0/executionState.hpp>
#include <backends/ascend/common.hpp>
#include <backends/ascend/L0/executionUnit.hpp>

namespace HiCR
{

namespace backend
{

namespace ascend
{

namespace L0
{

/**
 * This class represents the execution state of a kernel for the ascend backend.
 * Since kernels are not preemptible, it does not offer suspend/resume functionality.
 */
class ExecutionState final : public HiCR::L0::ExecutionState
{
  public:

  /**
   * Constructor for an ascend execution state
   *
   * \param executionUnit execution unit containing the kernel to execute
   * \param context ACL context associated to the device
   * \param deviceId ascend device id
   */
  ExecutionState(const HiCR::L0::ExecutionUnit *executionUnit, const aclrtContext context, const deviceIdentifier_t deviceId) : HiCR::L0::ExecutionState(executionUnit), _context(context), _deviceId(deviceId)
  {
    // Getting up-casted pointer for the execution unit
    auto e = dynamic_cast<const ExecutionUnit *>(executionUnit);

    // Checking whether the execution unit passed is compatible with this backend
    if (e == NULL) HICR_THROW_LOGIC("The execution unit of type '%s' is not supported by this backend\n", executionUnit->getType());

    _executionUnit = e;

    aclError err = aclrtCreateEvent(&_syncEvent);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not create synchronize bit");
  }

  ~ExecutionState()
  {
    aclError err = aclrtDestroyEvent(_syncEvent);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Failed to free synchronize bit");
  };

  /**
   * Synchronize and destroy the currently used stream
   */
  __USED__ inline void finalizeStream()
  {
    if (_isStreamActive)
    {
      // synchronize on the stream
      aclError err = aclrtSynchronizeStream(_stream);
      if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Failed to synchronize stream after kernel execution. Error %d", err);

      // destroy the stream
      err = aclrtDestroyStream(_stream);
      if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Failed to delete the stream after kernel execution. Error %d", err);

      // avoid deleting the stream more than once
      _isStreamActive = false;
    }
  }

  protected:

  /**
   * Internal implementation of resume routine.
   */
  __USED__ inline void resumeImpl() override
  {
    // select the ascend card
    selectDevice(_context, _deviceId);

    // Use FAST_LAUNCH option since the stream is meant to execute a sequence of kernels
    // that reuse the same stream
    aclError err = aclrtCreateStreamWithConfig(&_stream, 0, ACL_STREAM_FAST_LAUNCH);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not create stream on device %d", _deviceId);

    _isStreamActive = true;

    // start the sequence of kernels execution
    _executionUnit->start(_stream);

    // add an event at the end of the operations to query its status and check for completion
    err = aclrtRecordEvent(_syncEvent, _stream);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("can not set sync bit to 1. Error %d", err);
  }

  __USED__ inline void suspendImpl()
  {
    HICR_THROW_RUNTIME("Suspend functionality not supported by ascend backend");
  }

  /**
   * Internal implementation of checkFinalization routine. It periodically query the ACL event on the stream to check for completion and
   * automatically deletes the stream once it completes.
   *
   * \return whether all the kernels described in the execution unit finished.
   */
  __USED__ inline bool checkFinalizationImpl() override
  {
    // Check if the event has been processed
    aclrtEventRecordedStatus status;
    aclError err = aclrtQueryEventStatus(_syncEvent, &status);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("failed to query event status. err %d", err);

    // check the synchronization event status for stream completion
    if (status == ACL_EVENT_RECORDED_STATUS_NOT_READY) return false;

    // synchronize the stream and destroy it
    finalizeStream();

    return true;
  }

  private:

  /**
   * ACL context associated to the ascend device
   */
  const aclrtContext _context;

  /**
   * Ascend device id
   */
  const deviceIdentifier_t _deviceId;
  /**
   * Execution unit containing the kernel operations to execute
   */
  const ExecutionUnit *_executionUnit;
  /**
   * Stream on which the execution unit kernels are scheduled.
   */
  aclrtStream _stream;

  /**
   * Synchronization event to check for stream completion
   */
  aclrtEvent _syncEvent;

  /**
   * Keep track of the stream status
   */
  bool _isStreamActive = false;
};

} // namespace L0

} // namespace ascend

} // namespace backend

} // namespace HiCR
