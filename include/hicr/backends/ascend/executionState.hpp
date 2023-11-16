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
#include <hicr/backends/ascend/common.hpp>
#include <hicr/backends/ascend/executionUnit.hpp>
#include <hicr/common/exceptions.hpp>
#include <hicr/executionState.hpp>

namespace HiCR
{

namespace backend
{

namespace ascend
{

/**
 * This class represents the execution state of a kernel for the ascend backend.
 * Since kernels are not preemptible, it does not offer suspend/resume functionality.
 */
class ExecutionState final : public HiCR::ExecutionState
{
  public:

  /**
   * Constructor for an ascend execution state
   *
   * \param executionUnit execution unit containing the kernel to execute
   * \param context ACL context associated to the device
   * \param deviceId ascend device id
   */
  ExecutionState(const HiCR::ExecutionUnit *executionUnit, const aclrtContext context, const deviceIdentifier_t deviceId) : HiCR::ExecutionState(executionUnit), _context(context), _deviceId(deviceId)
  {
    // Getting up-casted pointer for the execution unit
    auto e = dynamic_cast<const ExecutionUnit *>(executionUnit);

    // Checking whether the execution unit passed is compatible with this backend
    if (e == NULL) HICR_THROW_LOGIC("The execution unit of type '%s' is not supported by this backend\n", executionUnit->getType());

    _executionUnit = e;

    // allocate synchronization variable
    aclError err = aclrtMallocHost((void **)&_synchronize, sizeof(int8_t));
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not create synchronize bit");
  }

  ~ExecutionState()
  {
    // free synchronization variable
    aclError err = aclrtFreeHost(_synchronize);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Failed to free synchronize bit");
  };

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

    // set the synchronize variable to 0
    err = aclrtMemset((void *)_synchronize, sizeof(int8_t), 0, sizeof(int8_t));
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not initialize synchronize bit");
    
    // start the sequence of kernels execution
    _executionUnit->start(_stream);

    // add a memset to the synchronization bit as the last operation on the stream.
    // This is a workaround to the stream query status
    err = aclrtMemsetAsync((void *)_synchronize, sizeof(int8_t), 1, sizeof(int8_t), _stream);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("can not set sync bit to 1. Error %d", err);
  }

  __USED__ inline void suspendImpl()
  {
    HICR_THROW_RUNTIME("Suspend functionality not supported by ascend backend");
  }

  /**
   * Internal implementation of checkFinalization routine. It periodically query the ACL stream to check for completion.
   *
   * \return whether all the kernels described in the execution unit finished.
   */
  __USED__ inline bool checkFinalizationImpl() override
  {
    // check the synchronization for stream completion
    if (*_synchronize == 0) return false;

    if (_isStreamActive)
    {
      // synchronize on the stream
      aclError err = aclrtSynchronizeStream(_stream);
      if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Failed to synchronize stream after kernel execution. Error %d", err);
      // destroy the stream
      err = aclrtDestroyStream(_stream);
      if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Failed to delete the stream after kernel execution. Error %d", err);

      _isStreamActive = false;
    }
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
   * Synchronization variable to check for stream completion
   */
  int8_t *_synchronize;

  /**
   * Keep track of the stream status
   */
  bool _isStreamActive = false;
};

} // end namespace ascend
} // namespace backend
} // namespace HiCR
