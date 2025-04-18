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
 * @brief This is a minimal backend for compute management of Ascend devices
 * @author S. M. Martin and L. Terracciano
 * @date 30/10/2023
 */

#pragma once

#include <acl/acl.h>
#include <memory>
#include <unordered_map>
#include <hicr/backends/ascend/executionUnit.hpp>
#include <hicr/backends/ascend/processingUnit.hpp>
#include <hicr/backends/ascend/kernel.hpp>
#include <hicr/core/computeManager.hpp>

namespace HiCR::backend::ascend
{

/**
 * Implementation of the HiCR Ascend backend compute manager.
 *
 * It stores the processing units detected by Ascend Computing Language.
 */
class ComputeManager final : public HiCR::ComputeManager
{
  public:

  /**
   * Constructor for the Compute Manager class for the Ascend backend
   */
  ComputeManager()
    : HiCR::ComputeManager(){};

  ~ComputeManager() override = default;

  /**
   * Creates an execution unit given a stream/vector of \p kernelOperations to be executed on the device
   *
   * \param[in] kernelOperations the sequence of kernel operations to executed
   *
   * \return a pointer to the new execution unit
   */
  __INLINE__ std::shared_ptr<HiCR::ExecutionUnit> createExecutionUnit(const std::vector<std::shared_ptr<ascend::Kernel>> &kernelOperations)
  {
    return std::make_shared<ExecutionUnit>(kernelOperations);
  }

  /**
   * Creates an execution state using the device context information and the exection unit to run on the Ascend
   *
   * \param[in] executionUnit execution Unit to launch on the ascend
   * \param[in] argument argument (not required by ascend)
   *
   * \return return a unique pointer to the newly created Execution State
   */
  __INLINE__ std::unique_ptr<HiCR::ExecutionState> createExecutionState(std::shared_ptr<HiCR::ExecutionUnit> executionUnit, void *const argument = nullptr) const override
  {
    return std::make_unique<ascend::ExecutionState>(executionUnit);
  }

  /**
   * Create a new processing unit for the specified \p resource (device)
   *
   * \param[in] resource the deviceId in which the processing unit is to be created
   *
   * \return a pointer to the new processing unit
   */
  __INLINE__ std::unique_ptr<HiCR::ProcessingUnit> createProcessingUnit(std::shared_ptr<HiCR::ComputeResource> resource) const override
  {
    return std::make_unique<ProcessingUnit>(resource);
  }

  protected:

  /**
   * Internal implementation of initailizeImpl
   * 
   * @param processingUnit the processing unit to operate on
   */
  __INLINE__ void initializeImpl(std::unique_ptr<HiCR::ProcessingUnit> &processingUnit) override
  {
    auto p = getAscendPointer(processingUnit);
    p->initialize();
  }

  /**
   * Internal implementation of initailizeImpl
   * 
   * @param processingUnit the processing unit to operate on
   * @param executionState the execution state to operate on
   */
  __INLINE__ void startImpl(std::unique_ptr<HiCR::ProcessingUnit> &processingUnit, std::unique_ptr<HiCR::ExecutionState> &executionState) override
  {
    auto p = getAscendPointer(processingUnit);
    p->start(executionState);
  }

  /**
   * Internal implementation of suspendImpl
   * 
   * @param processingUnit the processing unit to operate on
   */
  __INLINE__ void suspendImpl(std::unique_ptr<HiCR::ProcessingUnit> &processingUnit) override { HICR_THROW_RUNTIME("Suspend functionality not supported by Ascend backend"); }

  /**
   * Internal implementation of resumeImpl
   * 
   * @param processingUnit the processing unit to operate on
   */
  __INLINE__ void resumeImpl(std::unique_ptr<HiCR::ProcessingUnit> &processingUnit) override { HICR_THROW_RUNTIME("Resume functionality not supported by Ascend backend"); }

  /**
   * Internal implementation of terminateImpl
   * 
   * @param processingUnit the processing unit to operate on
   */
  __INLINE__ void terminateImpl(std::unique_ptr<HiCR::ProcessingUnit> &processingUnit) override {}

  /**
   * Internal implementation of awaitImpl
   * 
   * @param processingUnit the processing unit to operate on
   */
  __INLINE__ void awaitImpl(std::unique_ptr<HiCR::ProcessingUnit> &processingUnit) override
  {
    auto p = getAscendPointer(processingUnit);
    p->await();
  }

  private:

  /**
   * Return a pointer to an ascend processing unit.
   * 
   * \param processingUnit HiCR processing unit
   * 
   * \return ascend processung unit
  */
  [[nodiscard]] __INLINE__ ascend::ProcessingUnit *getAscendPointer(std::unique_ptr<HiCR::ProcessingUnit> &processingUnit)
  {
    // We can only handle processing units of Ascend type. Make sure we got the correct one
    // To make it fast and avoid string comparisons, we use the dynamic cast method
    auto p = dynamic_cast<ascend::ProcessingUnit *>(processingUnit.get());

    // If the processing unit is not recognized, throw error. We can use the processing unit's type (string) now.
    if (p == nullptr) HICR_THROW_LOGIC("This compute manager cannot handle processing units of type '%s'", processingUnit->getType());

    // Returning converted pointer
    return p;
  }
};

} // namespace HiCR::backend::ascend