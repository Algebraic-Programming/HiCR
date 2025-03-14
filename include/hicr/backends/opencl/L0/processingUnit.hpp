/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file processingUnit.hpp
 * @brief Implements the processing unit class for the opencl backend.
 * @author L. Terracciano 
 * @date 07/03/2025
 */

#pragma once

#include <memory>
#include <CL/opencl.hpp>
#include <hicr/core/exceptions.hpp>
#include <hicr/core/L0/processingUnit.hpp>
#include <hicr/backends/opencl/L0/computeResource.hpp>
#include <hicr/backends/opencl/L0/executionState.hpp>
#include <hicr/backends/opencl/L0/executionUnit.hpp>

namespace HiCR::backend::opencl::L1
{
/**
 * Forward declaration of the opencl device class -- a not-so-elegant solution to a circular dependency, but all we can do for now
 */
class ComputeManager;
} // namespace HiCR::backend::opencl::L1

namespace HiCR::backend::opencl::L0
{

/**
 * Implementation of a processing unit (a device capable of executing kernels) for the opencl backend
 */
class ProcessingUnit final : public HiCR::L0::ProcessingUnit
{
  friend class HiCR::backend::opencl::L1::ComputeManager;

  public:

  [[nodiscard]] __INLINE__ std::string getType() override { return "OpenCL Device"; }

  /**
   * Constructor for the Processing Unit (kernel) class
   *
   * \param computeResource The compute resource from which this processing unit is instantiated
   * \param context the OpenCL context
   */
  __INLINE__ ProcessingUnit(const std::shared_ptr<HiCR::L0::ComputeResource> &computeResource, const std::shared_ptr<cl::Context> &context)
    : HiCR::L0::ProcessingUnit(computeResource),
      _context(context)
  {
    // Getting up-casted pointer for the instance
    auto c = dynamic_pointer_cast<opencl::L0::ComputeResource>(computeResource);
    // Checking whether the execution unit passed is compatible with this backend
    if (c == NULL) HICR_THROW_LOGIC("The passed compute resource is not supported by this processing unit type\n");
  };

  private:

  /**
   * OpenCL context of the device
   */
  const std::shared_ptr<cl::Context> &_context;

  /**
   * Variable to hold the execution state to run
   */
  std::unique_ptr<ExecutionState> _executionState;

  /**
   * Initialize the processing unit
   */
  __INLINE__ void initialize() {}

  /**
   * OpenCL backend implementation that starts the execution state in the processing unit.
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
    auto c = static_cast<opencl::L0::ComputeResource *>(getComputeResource().get());

    // Set the device
    _executionState->setDevice(c->getDevice().lock());

    // Set context
    _executionState->setContext(_context);

    // Staring execution state
    _executionState.get()->resume();
  }

  /**
   * OpenCL backend implementation that wait for execution state completion
   */
  __INLINE__ void await()
  {
    // force the execution state to finalize
    _executionState.get()->finalizeStream();
  }

  [[nodiscard]] __INLINE__ opencl::L0::ExecutionState *getOpenCLExecutionStatePointer(std::unique_ptr<HiCR::L0::ExecutionState> &executionState)
  {
    // We can only handle execution state of OpenCL type. Make sure we got the correct one
    // To make it fast and avoid string comparisons, we use the dynamic cast method
    auto p = dynamic_cast<opencl::L0::ExecutionState *>(executionState.get());

    // If the execution state is not recognized, throw error
    if (p == nullptr) HICR_THROW_LOGIC("Execution state is not of type OpenCL");

    // Returning converted pointer
    return p;
  }
};

} // namespace HiCR::backend::opencl::L0