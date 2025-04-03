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
 * @file computationKernel.hpp
 * @brief This file implements the computation kernel class for the opencl backend
 * @author L. Terracciano
 * @date 07/03/2025
 */

#pragma once

#include <memory>
#include <vector>
#include <unordered_set>
#include <CL/opencl.hpp>
#include <hicr/core/L0/localMemorySlot.hpp>
#include <hicr/core/exceptions.hpp>
#include <hicr/backends/opencl/L0/localMemorySlot.hpp>
#include <hicr/backends/opencl/kernel.hpp>

namespace HiCR
{

namespace backend
{

namespace opencl
{

/**
 * This class represents a replicable Computation Kernel for the opencl backend.
 * A Computation Kernel enables the kernel execution in the HiCR runtime, and in particular enables
 * the concatenation of kernel execution and memcpy operations in a common queue of operations.
 */
class ComputationKernel final : public Kernel
{
  public:

  /**
   * Constructor for the Computation Kernel unit class of the opencl backend.
   * Set the kernel arguments.
   *
   * \param kernel OpenCL kernel
   * \param args kernel arguments
   * \param offset 
   * \param global 
   * \param local 
   */
  ComputationKernel(const std::shared_ptr<cl::Kernel>                             &kernel,
                    const std::vector<std::shared_ptr<HiCR::L0::LocalMemorySlot>> &args,
                    const cl::NDRange                                              offset,
                    const cl::NDRange                                              global,
                    const cl::NDRange                                              local)
    : Kernel(),
      _kernel(kernel),
      _offset(offset),
      _global(global),
      _local(local),
      _args(args)
  {
    for (size_t i = 0; i < _args.size(); i++)
    {
      auto a = getArgument(i);
      // Set the kernel argument
      auto err = _kernel->setArg<cl::Buffer>(i, *(a->getBuffer()));
      if (err != CL_SUCCESS) [[unlikely]] { HICR_THROW_RUNTIME("Can not set kernel arg. Error: %d", err); }
    }
  };

  ComputationKernel()  = delete;
  ~ComputationKernel() = default;

  /**
   * Start the kernel using the given OpenCL \p queue
   *
   * \param queue the OpenCL queue on which the kernel is to be executed
   */
  __INLINE__ void start(const cl::CommandQueue *queue) override
  {
    std::unordered_set<std::shared_ptr<HiCR::backend::opencl::L0::LocalMemorySlot>> unmappedSlots({});

    for (size_t i = 0; i < _args.size(); i++)
    {
      // Get the casted argument
      auto a = getArgument(i);

      // If already unmapped go to the next one
      if (unmappedSlots.contains(a) == true) { continue; }

      // Unmap all the arguments
      auto err = queue->enqueueUnmapMemObject(*(a->getBuffer()), a->getPointer());
      if (err != CL_SUCCESS) [[unlikely]] { HICR_THROW_RUNTIME("Can not unmap kernel arg. Error: %d", err); }

      // Add the slot to the unmapped set
      unmappedSlots.insert(a);
    }

    // start the kernel
    auto err = queue->enqueueNDRangeKernel(*_kernel, _offset, _global, _local);
    if (err != CL_SUCCESS) [[unlikely]] { HICR_THROW_RUNTIME("Failed to run the kernel. Error %d", err); }

    for (size_t i = 0; i < _args.size(); i++)
    {
      // Get the casted argument
      auto a = getArgument(i);

      if (unmappedSlots.empty() == true) { break; }
      if (unmappedSlots.contains(a) == false) { continue; }

      // Map arguments after kernel execution
      a->getPointer() = queue->enqueueMapBuffer(*(a->getBuffer()), CL_TRUE, CL_MAP_READ | CL_MAP_WRITE, 0, a->getSize(), nullptr, nullptr, &err);
      if (err != CL_SUCCESS) [[unlikely]] { HICR_THROW_RUNTIME("Can not map kernel arg. Error: %d", err); }

      // Remove the slot from the unmapped set
      unmappedSlots.erase(a);
    }
  }

  private:

  /**
   * OpenCL Kernel
   */
  std::shared_ptr<cl::Kernel> _kernel;

  /**
   * OpenCL offset
   */
  const cl::NDRange _offset;

  /**
   * OpenCL global
   */
  const cl::NDRange _global;

  /**
   * OpenCL local
   */
  const cl::NDRange _local;

  /**
   * Kernel arguments
  */
  const std::vector<std::shared_ptr<HiCR::L0::LocalMemorySlot>> _args;

  /**
   * Get the i-th argument casted as a opencl-specific memory slot
   * 
   * \param[in] i the argument index
   * 
   * \return opencl-specific memory slot
  */
  __INLINE__ std::shared_ptr<opencl::L0::LocalMemorySlot> getArgument(uint64_t i)
  {
    const auto &arg = _args[i];
    auto        a   = dynamic_pointer_cast<opencl::L0::LocalMemorySlot>(arg);
    if (a == nullptr) [[unlikely]] { HICR_THROW_RUNTIME("Provided memory slot containing the argument is not supported."); }
    return a;
  }
};

} // namespace opencl

} // namespace backend

} // namespace HiCR
