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
 * @brief This is a minimal backend for compute management of acl devices
 * @author S. M. Martin and L. Terracciano
 * @date 30/10/2023
 */

#pragma once

#include <acl/acl.h>
#include <memory>
#include <unordered_map>
#include <hicr/backends/acl/executionUnit.hpp>
#include <hicr/backends/acl/processingUnit.hpp>
#include <hicr/backends/acl/kernel.hpp>
#include <hicr/core/computeManager.hpp>

namespace HiCR::backend::acl
{

/**
 * Implementation of the HiCR acl backend compute manager.
 *
 * It stores the processing units detected by acl.
 */
class ComputeManager final : public HiCR::ComputeManager
{
  public:

  /**
   * Constructor for the Compute Manager class for the acl backend
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
  __INLINE__ std::shared_ptr<HiCR::ExecutionUnit> createExecutionUnit(const std::vector<std::shared_ptr<acl::Kernel>> &kernelOperations)
  {
    return std::make_shared<ExecutionUnit>(kernelOperations);
  }

  /**
   * Creates an execution state using the device context information and the exection unit to be run
   *
   * \param[in] executionUnit execution Unit to launch on the acl
   * \param[in] argument argument (not required by acl)
   *
   * \return return a unique pointer to the newly created Execution State
   */
  __INLINE__ std::unique_ptr<HiCR::ExecutionState> createExecutionState(std::shared_ptr<HiCR::ExecutionUnit> executionUnit, void *const argument = nullptr) const override
  {
    return std::make_unique<acl::ExecutionState>(executionUnit);
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
    auto p = getACLPointer(processingUnit);
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
    auto p = getACLPointer(processingUnit);
    p->start(executionState);
  }

  /**
   * Internal implementation of suspendImpl
   * 
   * @param processingUnit the processing unit to operate on
   */
  __INLINE__ void suspendImpl(std::unique_ptr<HiCR::ProcessingUnit> &processingUnit) override { HICR_THROW_RUNTIME("Suspend functionality not supported by acl backend"); }

  /**
   * Internal implementation of resumeImpl
   * 
   * @param processingUnit the processing unit to operate on
   */
  __INLINE__ void resumeImpl(std::unique_ptr<HiCR::ProcessingUnit> &processingUnit) override { HICR_THROW_RUNTIME("Resume functionality not supported by acl backend"); }

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
    auto p = getACLPointer(processingUnit);
    p->await();
  }

  private:

  /**
   * Return a pointer to an acl processing unit.
   * 
   * \param processingUnit HiCR processing unit
   * 
   * \return acl processung unit
  */
  [[nodiscard]] __INLINE__ acl::ProcessingUnit *getACLPointer(std::unique_ptr<HiCR::ProcessingUnit> &processingUnit)
  {
    // We can only handle processing units of acl type. Make sure we got the correct one
    // To make it fast and avoid string comparisons, we use the dynamic cast method
    auto p = dynamic_cast<acl::ProcessingUnit *>(processingUnit.get());

    // If the processing unit is not recognized, throw error. We can use the processing unit's type (string) now.
    if (p == nullptr) HICR_THROW_LOGIC("This compute manager cannot handle processing units of type '%s'", processingUnit->getType());

    // Returning converted pointer
    return p;
  }
};

} // namespace HiCR::backend::acl