/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
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

namespace HiCR
{

namespace backend
{

namespace opencl
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

} // namespace opencl

} // namespace backend

} // namespace HiCR
