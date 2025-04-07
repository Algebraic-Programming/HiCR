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
 * @brief This is a minimal backend for compute management of OpenCL devices
 * @author L. Terracciano
 * @date 07/03/2025
 */

#pragma once

#include <CL/opencl.hpp>
#include <memory>
#include <unordered_map>
#include <hicr/backends/opencl/executionUnit.hpp>
#include <hicr/backends/opencl/processingUnit.hpp>
#include <hicr/backends/opencl/kernel.hpp>
#include <hicr/core/computeManager.hpp>

namespace HiCR::backend::opencl
{

/**
 * Implementation of the HiCR opencl backend compute manager.
 *
 * It stores the processing units detected by OpenCL.
 */
class ComputeManager final : public HiCR::ComputeManager
{
  public:

  /**
   * Constructor for the Compute Manager class for the opencl backend
   * 
   * \param[in] context the OpenCL context
   */
  ComputeManager(const std::shared_ptr<cl::Context> &context)
    : HiCR::ComputeManager(),
      _context(context){};

  ~ComputeManager() override = default;

  /**
   * Creates an execution unit given a stream/vector of \p kernelOperations to be executed on the device
   *
   * \param[in] kernelOperations the sequence of kernel operations to executed
   *
   * \return a pointer to the new execution unit
   */
  __INLINE__ std::shared_ptr<HiCR::ExecutionUnit> createExecutionUnit(const std::vector<std::shared_ptr<opencl::Kernel>> &kernelOperations)
  {
    return std::make_shared<ExecutionUnit>(kernelOperations);
  }

  /**
   * Creates an execution state using the device context information and the exection unit to run on the opencl
   *
   * \param[in] executionUnit execution Unit to launch on the opencl
   * \param[in] argument argument (not required by opencl)
   *
   * \return return a unique pointer to the newly created Execution State
   */
  __INLINE__ std::unique_ptr<HiCR::ExecutionState> createExecutionState(std::shared_ptr<HiCR::ExecutionUnit> executionUnit, void *const argument = nullptr) const override
  {
    return std::make_unique<opencl::ExecutionState>(executionUnit);
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
    return std::make_unique<ProcessingUnit>(resource, _context);
  }

  protected:

  /**
   * Internal implementation of initailizeImpl
   * 
   * @param processingUnit the processing unit to operate on
   */
  __INLINE__ void initializeImpl(std::unique_ptr<HiCR::ProcessingUnit> &processingUnit) override
  {
    auto p = getOpenCLPointer(processingUnit);
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
    auto p = getOpenCLPointer(processingUnit);
    p->start(executionState);
  }

  /**
   * Internal implementation of suspendImpl
   * 
   * @param processingUnit the processing unit to operate on
   */
  __INLINE__ void suspendImpl(std::unique_ptr<HiCR::ProcessingUnit> &processingUnit) override { HICR_THROW_RUNTIME("Suspend functionality not supported by opencl backend"); }

  /**
   * Internal implementation of resumeImpl
   * 
   * @param processingUnit the processing unit to operate on
   */
  __INLINE__ void resumeImpl(std::unique_ptr<HiCR::ProcessingUnit> &processingUnit) override { HICR_THROW_RUNTIME("Resume functionality not supported by opencl backend"); }

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
    auto p = getOpenCLPointer(processingUnit);
    p->await();
  }

  private:

  const std::shared_ptr<cl::Context> &_context;

  [[nodiscard]] __INLINE__ opencl::ProcessingUnit *getOpenCLPointer(std::unique_ptr<HiCR::ProcessingUnit> &processingUnit)
  {
    // We can only handle processing units of OpenCL type. Make sure we got the correct one
    // To make it fast and avoid string comparisons, we use the dynamic cast method
    auto p = dynamic_cast<opencl::ProcessingUnit *>(processingUnit.get());

    // If the processing unit is not recognized, throw error. We can use the processing unit's type (string) now.
    if (p == nullptr) HICR_THROW_LOGIC("This compute manager cannot handle processing units of type '%s'", processingUnit->getType());

    // Returning converted pointer
    return p;
  }
};

} // namespace HiCR::backend::opencl