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
 * @file computeManager.hpp
 * @brief This file implements the compute manager for Pthreads backend
 * @authors S. M. Martin, O. Korakitis
 * @date 10/12/2023
 */

#pragma once

#include <memory>
#include <hicr/core/exceptions.hpp>
#include <hicr/backends/hwloc/computeResource.hpp>
#include <hicr/backends/pthreads/executionState.hpp>
#include <hicr/backends/pthreads/processingUnit.hpp>
#include <hicr/core/computeManager.hpp>

namespace HiCR::backend::pthreads
{

/**
 * Implementation of the pthreads compute manager.
 */
class ComputeManager : public HiCR::ComputeManager
{
  public:

  /**
   * Compute Manager constructor
   */
  ComputeManager()
    : HiCR::ComputeManager()
  {}

  /**
   * Default destructor
   */
  ~ComputeManager() override = default;

  /** This function enables the creation of an execution unit.
   *
   * Its default constructor takes a simple replicable CPU-executable function
   *
   * \param[in] function The replicable function to execute
   * @return The newly created execution unit
   */
  __INLINE__ std::shared_ptr<HiCR::ExecutionUnit> createExecutionUnit(const replicableFc_t &function) override { return std::make_shared<pthreads::ExecutionUnit>(function); }

  /**
   * Creates an execution state from an execution unit
   * 
   * @param[in] executionUnit The execution unit to run
   * @param[in] argument Argument to pass to the execution unit
   * @return A newly created execution state
   */
  __INLINE__ std::unique_ptr<HiCR::ExecutionState> createExecutionState(std::shared_ptr<HiCR::ExecutionUnit> executionUnit, void *const argument = nullptr) const override
  {
    // Creating and returning new execution state
    return std::make_unique<pthreads::ExecutionState>(executionUnit, argument);
  }

  [[nodiscard]] __INLINE__ std::unique_ptr<HiCR::ProcessingUnit> createProcessingUnit(std::shared_ptr<HiCR::ComputeResource> computeResource) const override
  {
    return std::make_unique<pthreads::ProcessingUnit>(computeResource);
  }

  private:

  __INLINE__ void initializeImpl(std::unique_ptr<HiCR::ProcessingUnit> &processingUnit) override
  {
    auto p = getPosixThreadPointer(processingUnit);

    // The logic for starting the posix thread is in the class itself
    p->initialize();
  }

  __INLINE__ void startImpl(std::unique_ptr<HiCR::ProcessingUnit> &processingUnit, std::unique_ptr<HiCR::ExecutionState> &executionState) override
  {
    auto p = getPosixThreadPointer(processingUnit);

    // The logic for starting the posix thread is in the class itself
    p->start(executionState);
  }

  __INLINE__ void suspendImpl(std::unique_ptr<HiCR::ProcessingUnit> &processingUnit) override
  {
    auto p = getPosixThreadPointer(processingUnit);

    // The logic for suspending the posix thread is in the class itself
    p->suspend();
  }

  __INLINE__ void resumeImpl(std::unique_ptr<HiCR::ProcessingUnit> &processingUnit) override
  {
    auto p = getPosixThreadPointer(processingUnit);

    // The logic for resuming the posix thread is in the class itself
    p->resume();
  }

  __INLINE__ void terminateImpl(std::unique_ptr<HiCR::ProcessingUnit> &processingUnit) override
  {
    auto p = getPosixThreadPointer(processingUnit);

    // The logic for resuming the posix thread is in the class itself
    p->terminate();
  }

  __INLINE__ void awaitImpl(std::unique_ptr<HiCR::ProcessingUnit> &processingUnit) override
  {
    auto p = getPosixThreadPointer(processingUnit);

    // The logic for awaiting the posix thread is in the class itself
    p->await();
  }

  [[nodiscard]] __INLINE__ pthreads::ProcessingUnit *getPosixThreadPointer(std::unique_ptr<HiCR::ProcessingUnit> &processingUnit)
  {
    // We can only handle processing units of Posix Thread type. Make sure we got the correct one
    // To make it fast and avoid string comparisons, we use the dynamic cast method
    auto p = dynamic_cast<pthreads::ProcessingUnit *>(processingUnit.get());

    // If the processing unit is not recognized, throw error. We can use the processing unit's type (string) now.
    if (p == nullptr) HICR_THROW_LOGIC("This compute manager cannot handle processing units of type '%s'", processingUnit->getType());

    // Returning converted pointer
    return p;
  }
};

} // namespace HiCR::backend::pthreads
