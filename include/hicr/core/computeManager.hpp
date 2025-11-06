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

/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file computeManager.hpp
 * @brief Provides a definition for the abstract compute manager class
 * @author S. M. Martin
 * @date 27/6/2023
 */

#pragma once

#include <memory>
#include <hicr/core/definitions.hpp>
#include <hicr/core/exceptions.hpp>
#include <hicr/core/computeResource.hpp>
#include <hicr/core/executionUnit.hpp>
#include <hicr/core/executionState.hpp>
#include <hicr/core/processingUnit.hpp>

namespace HiCR
{

/**
 * This class represents an abstract definition of a compute manager. That is, the set of functions to be implemented by a given backend that allows
 * the discovery of compute resources, the definition of replicable execution units (functions or kernels to run), and the instantiation of
 * execution states, that represent the execution lifetime of an execution unit.
 */
class ComputeManager
{
  public:

  /**
   * Default destructor
   */
  virtual ~ComputeManager() = default;

  /** This function enables the creation of an execution unit.
   *
   * Its default constructor takes a simple replicable CPU-executable function
   *
   * \param[in] function The replicable function to execute
   * @return The newly created execution unit
   */
  virtual __INLINE__ std::shared_ptr<HiCR::ExecutionUnit> createExecutionUnit(const replicableFc_t &function)
  {
    HICR_THROW_RUNTIME("This compute manager cannot create execution units out of replicable CPU-executable functions");
  }

  /**
   * Creates a new processing unit from the provided compute resource
   *
   * \param[in] resource This is the identifier of the compute resource to use to instantiate into a processing unit. The identifier should be one of those provided by the backend. Providing an arbitrary identifier may lead to unexpected behavior.
   *
   * @return A unique pointer to the newly created processing unit. It is important to preserve the uniqueness of this object, since it represents a physical resource (e.g., core) and we do not want to assign it to multiple workers.
   */
  [[nodiscard]] virtual std::unique_ptr<HiCR::ProcessingUnit> createProcessingUnit(std::shared_ptr<HiCR::ComputeResource> resource) const = 0;

  /**
   * This function enables the creation of an empty execution state object.
   *
   * The instantiation of its internal memory structures is delayed until explicit initialization to reduce memory usage when, for example, scheduling many tasks that do not need to execute at the same time.
   *
   * \param[in] executionUnit The replicable state-less execution unit to instantiate into an execution state
   * \param[in] argument Argument (closure) to pass to the execution unit to make this execution state unique
   * \return A unique pointer to the newly create execution state. It needs to be unique because the state cannot be simultaneously executed my multiple processing units
   */
  virtual std::unique_ptr<HiCR::ExecutionState> createExecutionState(std::shared_ptr<HiCR::ExecutionUnit> executionUnit, void *const argument = nullptr) const = 0;

  /**
   * Initializes the a processing unit and leaves it ready to execute work
   * 
   * @param[in] processingUnit The procesing unit to initialize
   */
  __INLINE__ void initialize(std::unique_ptr<HiCR::ProcessingUnit> &processingUnit)
  {
    // Getting processing unit's internal state
    auto state = processingUnit->getState();

    // Checking internal state
    if (state != HiCR::ProcessingUnit::uninitialized && state != HiCR::ProcessingUnit::terminated)
      HICR_THROW_RUNTIME("Attempting to initialize already initialized processing unit");

    // Calling PU-specific initialization
    initializeImpl(processingUnit);

    // Transitioning state
    processingUnit->setState(HiCR::ProcessingUnit::ready);
  }

  /**
   * Starts running an executing state on a processing unit 
   *
   * @param[in] processingUnit The processing unit to initiate computation with
   * @param[in] executionState The execution state to start running with the given processing unit
   */
  __INLINE__ void start(std::unique_ptr<HiCR::ProcessingUnit> &processingUnit, std::unique_ptr<HiCR::ExecutionState> &executionState)
  {
    // Getting processing unit's internal state
    auto state = processingUnit->getState();

    // Checking internal state
    if (state != HiCR::ProcessingUnit::ready) HICR_THROW_RUNTIME("Attempting to start processing unit that is not in the 'ready' state");

    // Transitioning state
    processingUnit->setState(HiCR::ProcessingUnit::running);

    // Running internal implementation of the start function
    startImpl(processingUnit, executionState);
  }

  /**
   * Triggers the suspension of the resource. All the elements that make the resource remain active in memory, but will not execute.
   * 
   * @param[in] processingUnit The processing unit to suspend
   */
  __INLINE__ void suspend(std::unique_ptr<HiCR::ProcessingUnit> &processingUnit)
  {
    // Getting processing unit's internal state
    auto state = processingUnit->getState();

    // Checking state
    if (state != HiCR::ProcessingUnit::running) HICR_THROW_RUNTIME("Attempting to suspend processing unit that is not in the 'running' state");

    // Transitioning state
    processingUnit->setState(HiCR::ProcessingUnit::suspended);

    // Calling internal implementation of the suspend function
    suspendImpl(processingUnit);
  }

  /**
   * Resumes the execution of the processing unit.
   * 
   * @param[in] processingUnit The processing unit to resume
   */
  __INLINE__ void resume(std::unique_ptr<HiCR::ProcessingUnit> &processingUnit)
  {
    // Getting processing unit's internal state
    auto state = processingUnit->getState();

    // Checking state
    if (state != HiCR::ProcessingUnit::suspended) HICR_THROW_RUNTIME("Attempting to resume processing unit that is not in the 'suspended' state");

    // Transitioning state
    processingUnit->setState(HiCR::ProcessingUnit::running);

    // Calling internal implementation of the resume function
    resumeImpl(processingUnit);
  }

  /**
   * Requests the processing unit to finalize as soon as possible. This is an asynchronous operation, so returning from this function does not guarantee that the resource has terminated.
   * 
   * @param[in] processingUnit The processing unit to terminate
   */
  __INLINE__ void terminate(std::unique_ptr<HiCR::ProcessingUnit> &processingUnit)
  {
    // Transitioning state
    processingUnit->setState(HiCR::ProcessingUnit::terminating);

    // Calling internal implementation of the terminate function
    terminateImpl(processingUnit);
  }

  /**
   * Suspends the execution of the caller until the given processing unit has finalized
   * 
   * @param[in] processingUnit The processing unit to wait for
   */
  __INLINE__ void await(std::unique_ptr<HiCR::ProcessingUnit> &processingUnit)
  {
    // Getting processing unit's internal state
    auto state = processingUnit->getState();

    // Checking state
    if (state != HiCR::ProcessingUnit::terminating && state != HiCR::ProcessingUnit::running && state != HiCR::ProcessingUnit::suspended) return;

    // Calling internal implementation of the await function
    awaitImpl(processingUnit);

    // Transitioning state
    processingUnit->setState(HiCR::ProcessingUnit::terminated);
  }

  protected:

  /**
   * Backend-specific implementation of the initialize function
   * 
   * @param[in] processingUnit The processing unit to initialize
   */
  virtual void initializeImpl(std::unique_ptr<HiCR::ProcessingUnit> &processingUnit) = 0;

  /**
  * Internal implmentation of the start function
  *
  * @param[in] processingUnit The processing unit to initiate computation with
  * @param[in] executionState The execution state to start running with the given processing unit
  */
  virtual void startImpl(std::unique_ptr<HiCR::ProcessingUnit> &processingUnit, std::unique_ptr<HiCR::ExecutionState> &executionState) = 0;

  /**
   * Internal implementation of the suspend function
   * 
   * @param[in] processingUnit The processing unit to suspend
   */
  virtual void suspendImpl(std::unique_ptr<HiCR::ProcessingUnit> &processingUnit) = 0;

  /**
   * Internal implementation of the resume function
   * 
   * @param[in] processingUnit The processing unit to resume
   */
  virtual void resumeImpl(std::unique_ptr<HiCR::ProcessingUnit> &processingUnit) = 0;

  /**
   * Internal implementation of the terminate function
   * 
   * @param[in] processingUnit The processing unit to terminate
   */
  virtual void terminateImpl(std::unique_ptr<HiCR::ProcessingUnit> &processingUnit) = 0;

  /**
   * Internal implementation of the await function
   * 
   * @param[in] processingUnit The processing to wait for
   */
  virtual void awaitImpl(std::unique_ptr<HiCR::ProcessingUnit> &processingUnit) = 0;
};
} // namespace HiCR
