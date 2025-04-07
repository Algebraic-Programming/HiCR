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
 * @brief Implements the processing unit class for the Ascend backend.
 * @author L. Terracciano & S. M. Martin
 * @date 1/11/2023
 */

#pragma once

#include <memory>
#include <hicr/core/exceptions.hpp>
#include <hicr/core/processingUnit.hpp>
#include <hicr/backends/ascend/computeResource.hpp>
#include <hicr/backends/ascend/executionState.hpp>
#include <hicr/backends/ascend/executionUnit.hpp>

namespace HiCR::backend::ascend
{
/**
 * Forward declaration of the Ascend device class -- a not-so-elegant solution to a circular dependency, but all we can do for now
 */
class ComputeManager;
} // namespace HiCR::backend::ascend

namespace HiCR::backend::ascend
{

/**
 * Implementation of a processing unit (a device capable of executing kernels) for the Ascend backend
 */
class ProcessingUnit final : public HiCR::ProcessingUnit
{
  friend class HiCR::backend::ascend::ComputeManager;

  public:

  /**
   * Constructor for the Processing Unit (kernel) class
   *
   * \param computeResource The compute resource from which this processing unit is instantiated
   */
  __INLINE__ ProcessingUnit(const std::shared_ptr<HiCR::ComputeResource> &computeResource)
    : HiCR::ProcessingUnit(computeResource)
  {
    // Getting up-casted pointer for the instance
    auto c = dynamic_pointer_cast<ascend::ComputeResource>(computeResource);

    // Checking whether the execution unit passed is compatible with this backend
    if (c == NULL) HICR_THROW_LOGIC("The passed compute resource is not supported by this processing unit type\n");

    // Select the device
    c->getDevice().lock()->select();

    // Use FAST_LAUNCH option since the stream is meant to execute a sequence of kernels
    // that reuse the same stream
    auto err = aclrtCreateStreamWithConfig(&_stream, 0, ACL_STREAM_FAST_LAUNCH);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Could not create stream. Error %d", err);
  };

  /**
   * Destructor. Destroys the stream
  */
  ~ProcessingUnit()
  {
    // destroy the stream
    auto err = aclrtDestroyStream(_stream);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Failed to delete the stream after kernel execution. Error %d", err);
  }

  /**
   * Get processing unit type
   * 
   * \return processing unit type
  */

  [[nodiscard]] __INLINE__ std::string getType() override { return "Ascend Device"; }

  private:

  /**
   * ACL stream
  */
  aclrtStream _stream;

  /**
   * Variable to hold the execution state to run
   */
  std::unique_ptr<ExecutionState> _executionState;

  /**
   * Initialize the processing unit
   */
  __INLINE__ void initialize() {}

  /**
   * Ascend backend implementation that starts the execution state in the processing unit.
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

    // Getting up-casted pointer for the instance
    auto c = static_cast<ascend::ComputeResource *>(getComputeResource().get());

    // select the curent Ascend card before starting the execution state
    c->getDevice().lock()->select();

    // set the stream in the execution state
    _executionState->setStream(_stream);

    // Staring execution state
    _executionState.get()->resume();
  }

  /**
   * Ascend backend implementation that wait for execution state completion
   */
  __INLINE__ void await()
  {
    // Getting up-casted pointer for the instance
    auto c = static_cast<ascend::ComputeResource *>(getComputeResource().get());

    // select the curent Ascend card before starting the execution state
    c->getDevice().lock()->select();

    // force the execution state to finalize
    _executionState.get()->finalizeStream();
  }

  [[nodiscard]] __INLINE__ ascend::ExecutionState *getAscendExecutionStatePointer(std::unique_ptr<HiCR::ExecutionState> &executionState)
  {
    // We can only handle execution state of Ascend type. Make sure we got the correct one
    // To make it fast and avoid string comparisons, we use the dynamic cast method
    auto p = dynamic_cast<ascend::ExecutionState *>(executionState.get());

    // If the execution state is not recognized, throw error
    if (p == nullptr) HICR_THROW_LOGIC("Execution state is not of type Ascend");

    // Returning converted pointer
    return p;
  }
};

} // namespace HiCR::backend::ascend