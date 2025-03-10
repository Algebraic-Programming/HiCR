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

namespace HiCR::backend::ascend::L1
{
/**
 * Forward declaration of the ascend device class -- a not-so-elegant solution to a circular dependency, but all we can do for now
 */
class ComputeManager;
} // namespace HiCR::backend::ascend::L1

namespace HiCR::backend::ascend::L0
{

/**
 * Implementation of a processing unit (a device capable of executing kernels) for the ascend backend
 */
class ProcessingUnit final : public HiCR::L0::ProcessingUnit
{
  friend class HiCR::backend::ascend::L1::ComputeManager;

  public:

  [[nodiscard]] __INLINE__ std::string getType() override { return "Ascend Device"; }

  /**
   * Constructor for the Processing Unit (kernel) class
   *
   * \param computeResource The compute resource from which this processing unit is instantiated
   */
  __INLINE__ ProcessingUnit(const std::shared_ptr<HiCR::L0::ComputeResource> &computeResource)
    : HiCR::L0::ProcessingUnit(computeResource)
  {
    // Getting up-casted pointer for the instance
    auto c = dynamic_pointer_cast<ascend::L0::ComputeResource>(computeResource);

    // Checking whether the execution unit passed is compatible with this backend
    if (c == NULL) HICR_THROW_LOGIC("The passed compute resource is not supported by this processing unit type\n");
  };

  private:

  /**
   * ACL context of the device
   */
  aclrtContext _context;

  /**
   * Variable to hold the execution state to run
   */
  std::unique_ptr<ExecutionState> _executionState;

  /**
   * Initialize the processing unit
   */
  __INLINE__ void initialize()
  {
    // Getting up-casted pointer for the instance
    auto c = dynamic_pointer_cast<ascend::L0::ComputeResource>(getComputeResource());

    // Checking whether the execution unit passed is compatible with this backend
    if (c == NULL) HICR_THROW_LOGIC("The passed compute resource is not supported by this processing unit type\n");

    // Get ACL context
    _context = c->getDevice().lock()->getContext();
  }

  /**
   * Ascend backend implementation that starts the execution state in the processing unit.
   *
   * \param executionState the execution state to start
   */
  __INLINE__ void start(std::unique_ptr<HiCR::L0::ExecutionState> &executionState)
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
   * Ascend backend implementation that wait for execution state completion
   */
  __INLINE__ void await()
  {
    // Getting up-casted pointer for the instance
    auto c = static_cast<ascend::L0::ComputeResource *>(getComputeResource().get());

    // select the curent Ascend card before starting the execution state
    c->getDevice().lock()->select();

    // force the execution state to finalize
    _executionState.get()->finalizeStream();
  }

  [[nodiscard]] __INLINE__ ascend::L0::ExecutionState *getAscendExecutionStatePointer(std::unique_ptr<HiCR::L0::ExecutionState> &executionState)
  {
    // We can only handle execution state of Ascend type. Make sure we got the correct one
    // To make it fast and avoid string comparisons, we use the dynamic cast method
    auto p = dynamic_cast<ascend::L0::ExecutionState *>(executionState.get());

    // If the execution state is not recognized, throw error
    if (p == nullptr) HICR_THROW_LOGIC("Execution state is not of type Ascend");

    // Returning converted pointer
    return p;
  }
};

} // namespace HiCR::backend::ascend::L0