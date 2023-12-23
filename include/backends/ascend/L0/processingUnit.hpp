/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file processingUnit.hpp
 * @brief Implements the processing unit class for the ascend backend.
 * @author L. Terracciano & S. M. Martin
 * @date 1/11/2023
 */

#pragma once

#include <backends/ascend/L0/computeResource.hpp>
#include <backends/ascend/L0/executionState.hpp>
#include <backends/ascend/L0/executionUnit.hpp>
#include <hicr/L0/processingUnit.hpp>
#include <hicr/common/exceptions.hpp>
#include <memory>

namespace HiCR
{

namespace backend
{

namespace ascend
{

namespace L0
{

/**
 * Implementation of a processing unit (a device capable of executing kernels) for the ascend backend
 */
class ProcessingUnit final : public HiCR::L0::ProcessingUnit
{
  public:

  /**
   * Constructor for the Processing Unit (kernel) class
   *
   * \param device device ID
   */
  __USED__ inline ProcessingUnit(HiCR::L0::ComputeResource *computeResource) : HiCR::L0::ProcessingUnit(computeResource)
  {
    // Getting up-casted pointer for the instance
    auto c = dynamic_cast<ascend::L0::ComputeResource *>(computeResource);

    // Checking whether the execution unit passed is compatible with this backend
    if (c == NULL) HICR_THROW_LOGIC("The passed compute resource is not supported by this processing unit type\n");
  };

  protected:

  /**
   * Internal implementation of initailizeImpl
   */
  __USED__ inline void initializeImpl() override
  {
    // Getting device id associated to the underlying compute resource (ascend)
    auto deviceId = ((ascend::L0::ComputeResource*)getComputeResource())->getDevice()->getId();

    aclError err = aclrtCreateContext(&_context, deviceId);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not create ACL context on device %d. Error %d", deviceId, err);
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
  __USED__ inline void startImpl(std::unique_ptr<HiCR::L0::ExecutionState> executionState) override
  {
    // Getting up-casted pointer for the execution unit
    std::unique_ptr<ExecutionState> e = std::unique_ptr<ExecutionState>(dynamic_cast<ExecutionState *>(executionState.release()));

    // Checking whether the execution unit passed is compatible with this backend
    if (e == NULL) HICR_THROW_LOGIC("The execution state is not supported by this backend\n");

    // Storing execution state object
    _executionState = std::move(e);

    // Getting up-casted pointer for the instance
    auto c = static_cast<ascend::L0::ComputeResource *>(getComputeResource());

    // select the curent Ascend card before starting the execution state
    c->getDevice()->select();

    // Staring execution state
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
    // Getting device id associated to the underlying compute resource (ascend)
    auto deviceId = ((ascend::L0::ComputeResource *)getComputeResource())->getDevice()->getId();

    // Getting up-casted pointer for the instance
    auto c = static_cast<ascend::L0::ComputeResource *>(getComputeResource());

    // select the curent Ascend card before starting the execution state
    c->getDevice()->select();
    
    // force the execution state to finalize
    _executionState.get()->finalizeStream();

    // destroy the ACL context
    aclError err = aclrtDestroyContext(_context);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Failed to destroy ACL context on device %d. Error %d", deviceId, err);
  }

  private:

  /**
   * ACL context of the device
   */
  aclrtContext _context;

  /**
   * Variable to hold the execution state to run
   */
  std::unique_ptr<ExecutionState> _executionState;
};

} // namespace L0

} // namespace ascend

} // namespace backend

} // namespace HiCR
