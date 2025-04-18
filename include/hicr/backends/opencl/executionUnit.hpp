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
 * @file executionUnit.hpp
 * @brief This file implements the execution unit class for the OpenCL backend
 * @author L. Terracciano
 * @date 07/03/2025
 */

#pragma once

#include <string>
#include <vector>
#include <CL/opencl.hpp>
#include <hicr/backends/opencl/kernel.hpp>
#include <hicr/core/executionUnit.hpp>

namespace HiCR::backend::opencl
{
/**
 * This class represents a replicable sequence of kernels meant to be executed on OpenCL.
 */
class ExecutionUnit final : public HiCR::ExecutionUnit
{
  public:

  /**
   * Constructor for the execution unit class of the OpenCL backend
   *
   * \param kernelOperations kernels to execute
   */
  ExecutionUnit(const std::vector<std::shared_ptr<opencl::Kernel>> &kernelOperations)
    : HiCR::ExecutionUnit(),
      _kernels(kernelOperations){};
  ExecutionUnit() = delete;

  /**
   * Default destructor
   */
  ~ExecutionUnit() = default;

  /**
   * Get the execution unit type
   *
   * \return a string indicating the execution unit type
   */
  __INLINE__ std::string getType() const override { return "OpenCL Kernel"; }

  /**
   * Start the sequence of kernels on the specified \p queue
   *
   * \param queue queue on which kernels are scheduled
   */
  __INLINE__ void start(const cl::CommandQueue *queue) const
  {
    for (auto &kernel : _kernels) kernel->start(queue);
  }

  private:

  /**
   * Ordered sequence of kernels meant to be executed as a unique stream of operations.
   */
  const std::vector<std::shared_ptr<opencl::Kernel>> _kernels;
};

} // namespace HiCR::backend::opencl
