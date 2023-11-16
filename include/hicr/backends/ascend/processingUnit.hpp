/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file processingUnit.hpp
 * @brief Implements the processing unit class for the ascend backend.
 * @author S. M. Martin & L. Terracciano
 * @date 1/11/2023
 */

#pragma once

#include <hicr/backends/ascend/executionState.hpp>
#include <hicr/backends/ascend/executionUnit.hpp>
#include <hicr/common/exceptions.hpp>
#include <hicr/processingUnit.hpp>

namespace HiCR
{

namespace backend
{

namespace ascend
{

/**
 * Implementation of a processing unit (a device capable of executing kernels) for the ascend backend
 */
class ProcessingUnit final : public HiCR::ProcessingUnit
{
  public:

  /**
   * Constructor for the Processing Unit (kernel) class
   *
   * \param device device ID
   * \param context ACL context for the desired device
   */
  __USED__ inline ProcessingUnit(computeResourceId_t device) : HiCR::ProcessingUnit(device), _deviceId(device){};

  /**
   * Creates an execution state using the device context information and the exection unit to run on the ascend
   *
   * \param executionUnit rxecution Unit to launch on the ascend
   *
   * \return return a unique pointer to the newly created Execution State
   */
  __USED__ inline std::unique_ptr<HiCR::ExecutionState> createExecutionState(HiCR::ExecutionUnit *executionUnit) override
  {
    return std::make_unique<ExecutionState>(executionUnit, _context, _deviceId);
  }

  protected:

  /**
   * Internal implementation of initailizeImpl
   */
  __USED__ inline void initializeImpl() override
  {
    aclError err = aclrtCreateContext(&_context, _deviceId);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not create ACL context on device %d. Error %d", _deviceId, err);
  }

  /**
   * Internal implementation of suspendImpl
   */
  __USED__ inline void suspendImpl() override
  {
    HICR_THROW_RUNTIME("Resume functionality not supported by ascend backend");
  }

  /**
   * Internal implementation of resumeImpl
   */
  __USED__ inline void resumeImpl() override
  {
    HICR_THROW_RUNTIME("Resume functionality not supported by ascend backend");
  }

  /**
   * Ascend backend implementation that starts the execution state in the processing unit.
   *
   * \param executionState the execution state to start
   */
  __USED__ inline void startImpl(std::unique_ptr<HiCR::ExecutionState> executionState) override
  {
    // Getting up-casted pointer for the execution unit
    std::unique_ptr<ExecutionState> e = std::unique_ptr<ExecutionState>(dynamic_cast<ExecutionState *>(executionState.release()));

    // Checking whether the execution unit passed is compatible with this backend
    if (e == NULL) HICR_THROW_LOGIC("The execution state is not supported by this backend\n");

    _executionState = std::move(e);
    _executionState.get()->resume();
  }

  /**
   * Internal implementation of terminateImpl
   */
  __USED__ inline void terminateImpl() override
  {
  }

  /**
   * Ascend backend implementation that wait for execution state completion
   */
  __USED__ inline void awaitImpl() override
  {
    // force the execution state to finalize
    // TODO: find another way to implement this. Sync?
    while(!_executionState.get()->checkFinalization()) {}
  
    // destroy the ACL context
    aclError err = aclrtDestroyContext(_context);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Failed to destroy ACL context on device %d. Error %d", _deviceId, err);
  }

  private:

  /**
   * Device Identifier
   */
  const deviceIdentifier_t _deviceId;

  /**
   * ACL context of the device
   */
  aclrtContext _context;

  /**
   * Variable to hold the execution state to run
   */
  std::unique_ptr<ExecutionState> _executionState;
};

} // namespace ascend

} // namespace backend

} // namespace HiCR
