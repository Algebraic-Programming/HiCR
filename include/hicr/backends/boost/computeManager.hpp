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
 * @brief This file implements the pthread-based compute manager for Boost backend
 * @authors S. M. Martin, O. Korakitis
 * @date 10/12/2023
 */

#pragma once

#include <memory>
#include <hicr/core/exceptions.hpp>
#include <hicr/backends/boost/executionState.hpp>
#include <hicr/core/computeManager.hpp>

namespace HiCR::backend::boost
{

/**
 * Implementation of the Boost compute manager.
 * 
 * \note The compute manager does not offer a processing unit implementation. However, the execution unit and states are fully compatible with the processing unit offered by pthreads backend 
 */
class ComputeManager : public HiCR::ComputeManager
{
  public:

  /**
   * Default constructor
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
  __INLINE__ std::shared_ptr<HiCR::ExecutionUnit> createExecutionUnit(const replicableFc_t &function) override { return std::make_shared<boost::ExecutionUnit>(function); }

  /**
   * Creates a new execution state (coroutine)
   * 
   * @param[in] executionUnit The (lambda) function to run
   * @param[in] argument The argument to pass to the lambda function
   * @return The newly created execution state
   */
  __INLINE__ std::unique_ptr<HiCR::ExecutionState> createExecutionState(std::shared_ptr<HiCR::ExecutionUnit> executionUnit, void *const argument = nullptr) const override
  {
    // Creating and returning new execution state
    return std::make_unique<boost::ExecutionState>(executionUnit, argument);
  }

  [[nodiscard]] __INLINE__ std::unique_ptr<HiCR::ProcessingUnit> createProcessingUnit(std::shared_ptr<HiCR::ComputeResource> computeResource) const override
  {
    {
      HICR_THROW_LOGIC("This backend does not implement this function");
    }
  }

  private:

  __INLINE__ void initializeImpl(std::unique_ptr<HiCR::ProcessingUnit> &processingUnit) override
  {
    {
      HICR_THROW_LOGIC("This backend does not implement this function");
    }
  }

  __INLINE__ void startImpl(std::unique_ptr<HiCR::ProcessingUnit> &processingUnit, std::unique_ptr<HiCR::ExecutionState> &executionState) override
  {
    {
      HICR_THROW_LOGIC("This backend does not implement this function");
    }
  }

  __INLINE__ void suspendImpl(std::unique_ptr<HiCR::ProcessingUnit> &processingUnit) override
  {
    {
      HICR_THROW_LOGIC("This backend does not implement this function");
    }
  }

  __INLINE__ void resumeImpl(std::unique_ptr<HiCR::ProcessingUnit> &processingUnit) override
  {
    {
      HICR_THROW_LOGIC("This backend does not implement this function");
    }
  }

  __INLINE__ void terminateImpl(std::unique_ptr<HiCR::ProcessingUnit> &processingUnit) override
  {
    {
      HICR_THROW_LOGIC("This backend does not implement this function");
    }
  }

  __INLINE__ void awaitImpl(std::unique_ptr<HiCR::ProcessingUnit> &processingUnit) override
  {
    {
      HICR_THROW_LOGIC("This backend does not implement this function");
    }
  }
};

} // namespace HiCR::backend::boost
