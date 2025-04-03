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
 * @brief This file implements the execution unit class for the ascend backend
 * @author L. Terracciano
 * @date 30/10/2023
 */

#pragma once

#include <acl/acl.h>
#include <string>
#include <vector>
#include <hicr/backends/ascend/kernel.hpp>
#include <hicr/core/L0/executionUnit.hpp>

namespace HiCR
{

namespace backend
{

namespace ascend
{

namespace L0
{

/**
 * This class represents a replicable sequence of kernels meant to be executed on Ascend.
 */
class ExecutionUnit final : public HiCR::L0::ExecutionUnit
{
  public:

  /**
   * Constructor for the execution unit class of the ascend backend
   *
   * \param kernelOperations kernels to execute
   */
  ExecutionUnit(const std::vector<std::shared_ptr<ascend::Kernel>> &kernelOperations)
    : HiCR::L0::ExecutionUnit(),
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
  __INLINE__ std::string getType() const override { return "Ascend Kernel"; }

  /**
   * Start the sequence of kernels on the specified \p stream
   *
   * \param stream stream on which kernels are scheduled
   */
  __INLINE__ void start(const aclrtStream stream) const
  {
    for (auto &kernel : _kernels) kernel->start(stream);
  }

  private:

  /**
   * Ordered sequence of kernels meant to be executed as a unique stream of operations.
   */
  const std::vector<std::shared_ptr<ascend::Kernel>> _kernels;
};

} // namespace L0

} // namespace ascend

} // namespace backend

} // namespace HiCR
