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

#include <memory>
#include <hicr/core/exceptions.hpp>
#include <hicr/core/L0/processingUnit.hpp>
#include <hicr/backends/ascend/L0/computeResource.hpp>
#include <hicr/backends/ascend/L0/executionState.hpp>
#include <hicr/backends/ascend/L0/executionUnit.hpp>

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
   * \param computeResource The compute resource from which this processing unit is instantiated
   */
  __INLINE__ ProcessingUnit(std::shared_ptr<HiCR::L0::ComputeResource> computeResource)
    : HiCR::L0::ProcessingUnit(computeResource)
  {
    // Getting up-casted pointer for the instance
    auto c = dynamic_pointer_cast<ascend::L0::ComputeResource>(computeResource);

    // Checking whether the execution unit passed is compatible with this backend
    if (c == NULL) HICR_THROW_LOGIC("The passed compute resource is not supported by this processing unit type\n");

    _context = c->getDevice().lock()->getContext();
  };

  protected:

  /**
   * Internal implementation of initailizeImpl
   */
  __INLINE__ void initializeImpl() override
  {
    // Getting device id associated to the underlying compute resource (ascend)
    auto deviceId = dynamic_pointer_cast<ascend::L0::ComputeResource>(getComputeResource())->getDevice().lock()->getId();

    aclError err = aclrtCreateContext(&_context, deviceId);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not create ACL context on device %d. Error %d", deviceId, err);
  }

  /**
   * Internal implementation of suspendImpl
   */
  __INLINE__ void suspendImpl() override { HICR_THROW_RUNTIME("Resume functionality not supported by ascend backend"); }

  /**
   * Internal implementation of resumeImpl
   */
  __INLINE__ void resumeImpl() override { HICR_THROW_RUNTIME("Resume functionality not supported by ascend backend"); }

  /**
   * Ascend backend implementation that starts the execution state in the processing unit.
   *
   * \param executionState the execution state to start
   */
  __INLINE__ void startImpl(std::unique_ptr<HiCR::L0::ExecutionState> executionState) override
  {
    // Getting up-casted pointer for the execution unit
    auto e = std::unique_ptr<ExecutionState>(dynamic_cast<ExecutionState *>(executionState.release()));

    // Checking whether the execution unit passed is compatible with this backend
    if (e == NULL) HICR_THROW_LOGIC("The execution state is not supported by this backend\n");

    // Storing execution state object
    _executionState = std::move(e);

    // Getting up-casted pointer for the instance
    auto c = static_cast<ascend::L0::ComputeResource *>(getComputeResource().get());

    // select the curent Ascend card before starting the execution state
    c->getDevice().lock()->select();

    // Staring execution state
    _executionState.get()->resume();
  }

  /**
   * Internal implementation of terminateImpl
   */
  __INLINE__ void terminateImpl() override {}

  /**
   * Ascend backend implementation that wait for execution state completion
   */
  __INLINE__ void awaitImpl() override
  {
    // Getting up-casted pointer for the instance
    auto c = static_cast<ascend::L0::ComputeResource *>(getComputeResource().get());

    // select the curent Ascend card before starting the execution state
    c->getDevice().lock()->select();

    // force the execution state to finalize
    _executionState.get()->finalizeStream();
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
