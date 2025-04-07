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
 * @file processingUnit.hpp
 * @brief Implements the processing unit class for the opencl backend.
 * @author L. Terracciano 
 * @date 07/03/2025
 */

#pragma once

#include <memory>
#include <CL/opencl.hpp>
#include <hicr/core/exceptions.hpp>
#include <hicr/core/processingUnit.hpp>
#include <hicr/backends/opencl/computeResource.hpp>
#include <hicr/backends/opencl/executionState.hpp>
#include <hicr/backends/opencl/executionUnit.hpp>

namespace HiCR::backend::opencl
{

/**
 * Forward declaration of the opencl device class -- a not-so-elegant solution to a circular dependency, but all we can do for now
 */
class ComputeManager;
} // namespace HiCR::backend::opencl

namespace HiCR::backend::opencl
{

/**
 * Implementation of a processing unit (a device capable of executing kernels) for the opencl backend
 */
class ProcessingUnit final : public HiCR::ProcessingUnit
{
  friend class HiCR::backend::opencl::ComputeManager;

  public:

  /**
   * Constructor for the Processing Unit (kernel) class
   *
   * \param computeResource The compute resource from which this processing unit is instantiated
   * \param context the OpenCL context
   */
  __INLINE__ ProcessingUnit(const std::shared_ptr<HiCR::ComputeResource> &computeResource, const std::shared_ptr<cl::Context> &context)
    : HiCR::ProcessingUnit(computeResource)
  {
    // Getting up-casted pointer for the instance
    auto c = dynamic_pointer_cast<opencl::ComputeResource>(computeResource);
    // Checking whether the execution unit passed is compatible with this backend
    if (c == NULL) HICR_THROW_LOGIC("The passed compute resource is not supported by this processing unit type\n");

    // Create queue
    _queue = std::make_unique<cl::CommandQueue>(*context, c->getDevice().lock()->getOpenCLDevice());
  };

  /**
   * Get processing unit type
   * 
   * \return processing unit type
  */
  [[nodiscard]] __INLINE__ std::string getType() override { return "OpenCL Device"; }

  private:

  /**
   * OpenCL command queue
   */
  std::unique_ptr<cl::CommandQueue> _queue;

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
  __INLINE__ void start(std::unique_ptr<HiCR::ExecutionState> &executionState)
  {
    // Getting up-casted pointer for the execution unit
    auto e = std::unique_ptr<ExecutionState>(dynamic_cast<ExecutionState *>(executionState.release()));

    // Checking whether the execution unit passed is compatible with this backend
    if (e == NULL) HICR_THROW_LOGIC("The execution state is not supported by this backend\n");

    // Storing execution state object
    _executionState = std::move(e);

    // Set context
    _executionState->setQueue(_queue.get());

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

  [[nodiscard]] __INLINE__ opencl::ExecutionState *getOpenCLExecutionStatePointer(std::unique_ptr<HiCR::ExecutionState> &executionState)
  {
    // We can only handle execution state of OpenCL type. Make sure we got the correct one
    // To make it fast and avoid string comparisons, we use the dynamic cast method
    auto p = dynamic_cast<opencl::ExecutionState *>(executionState.get());

    // If the execution state is not recognized, throw error
    if (p == nullptr) HICR_THROW_LOGIC("Execution state is not of type OpenCL");

    // Returning converted pointer
    return p;
  }
};

} // namespace HiCR::backend::opencl