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
 * @file kernel.hpp
 * @brief This file implements the Kernel class for the OpenCL backend
 * @author L. Terracciano
 * @date 07/03/2025
 */

#pragma once

#include <CL/opencl.hpp>
#include <hicr/core/definitions.hpp>

namespace HiCR::backend::opencl
{

/**
 * This class represents a replicable kernel for the OpenCL backend.
 * A Kernel is a piece of computation meant to be executed on an OpenCL device.
 */
class Kernel
{
  public:

  /**
   * Default constructor
  */
  __INLINE__ Kernel() = default;

  /**
   * Default destructor
   */
  __INLINE__ ~Kernel() = default;

  /**
   * Start the execution of the kernel on the given \p queue
   *
   * \param queue the queue on which the kernel is started
   */
  __INLINE__ virtual void start(const cl::CommandQueue *queue) = 0;
};

} // namespace HiCR::backend::opencl
