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
 * @brief Provides a definition for a HiCR ProcessingUnit class
 * @author S. M. Martin
 * @date 13/7/2023
 */
#pragma once

#include <memory>
#include <hicr/core/definitions.hpp>
#include <hicr/core/computeResource.hpp>
#include <hicr/core/executionUnit.hpp>
#include <hicr/core/executionState.hpp>
#include <utility>

namespace HiCR
{
class ComputeManager;
}

namespace HiCR
{

/**
 * This class represents an abstract definition for a Processing Unit resource in HiCR that:
 *
 * - Represents a single compute resource that has been instantiated for execution (as opposed of those who shall remain unused or unassigned).
 * - Is capable of executing or contributing to the execution of tasks.
 * - Is assigned, for example, to a worker to perform the work necessary to execute a task.
 * - This is a non-copy-able class
 */
class ProcessingUnit
{
  friend class HiCR::ComputeManager;

  public:

  /**
   * Complete state set that a worker can be in
   */
  enum state_t
  {
    /**
     * The worker object has been instantiated but not initialized
     */
    uninitialized,

    /**
     * The worker has been ininitalized (or is back from executing) and can currently run
     */
    ready,

    /**
     * The worker has started executing
     */
    running,

    /**
     * The worker has started executing
     */
    suspended,

    /**
     * The worker has been issued for termination (but still running)
     */
    terminating,

    /**
     * The worker has terminated
     */
    terminated
  };

  /**
   * Disabled default constructor
   */
  ProcessingUnit() = delete;

  /**
   * Constructor for a processing unit
   *
   * \param computeResource The instance of the compute resource to instantiate, as indicated by the backend
   */
  __INLINE__ ProcessingUnit(std::shared_ptr<HiCR::ComputeResource> computeResource)
    : _computeResource(std::move(computeResource)){};

  /**
   * Default destructor
  */
  virtual ~ProcessingUnit() = default;

  /**
   * Function to obtain the processing unit's state
   *
   * \return Retruns the current state
   */
  [[nodiscard]] __INLINE__ ProcessingUnit::state_t getState() const { return _state; }

  /**
   * Returns the processing unit's associated compute resource
   *
   * \return The identifier of the compute resource associated to this processing unit.
   */
  __INLINE__ std::shared_ptr<ComputeResource> getComputeResource() { return _computeResource; }

  /**
   * Gets the processing unit's type
   *
   * \return A string, containing the processing unit's type
   */
  virtual std::string getType() = 0;

  private:

  /**
   * Function to set the processing unit's state
   *
   * @param state new state to set
   */
  __INLINE__ void setState(const ProcessingUnit::state_t state) { _state = state; }

  /**
   * Represents the internal state of the processing unit. Uninitialized upon construction.
   */
  ProcessingUnit::state_t _state = ProcessingUnit::uninitialized;

  /**
   * Identifier of the compute resource associated to this processing unit
   */
  const std::shared_ptr<HiCR::ComputeResource> _computeResource;
};

} // namespace HiCR
