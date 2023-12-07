/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file executionUnit.hpp
 * @brief This file implements the execution unit class for the ascend backend
 * @author S. M. Martin & L. Terracciano
 * @date 30/10/2023
 */

#pragma once

#include <acl/acl.h>
#include <chrono>
#include <hicr/backends/ascend/kernel.hpp>
#include <hicr/L0/executionUnit.hpp>
#include <thread>
#include <vector>

namespace HiCR
{

namespace backend
{

namespace ascend
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
  ExecutionUnit(const std::vector<kernel::Kernel *> &kernelOperations) : HiCR::L0::ExecutionUnit(), _kernels(kernelOperations){};
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
  __USED__ inline std::string getType() const override { return "Ascend Kernel"; }

  /**
   * Start the sequence of kernels on the specified \p stream
   *
   * \param stream stream on which kernels are scheduled
   */
  __USED__ inline void start(const aclrtStream stream) const
  {
    for (auto kernel : _kernels) kernel->start(stream);
  }

  private:

  /**
   * Ordered sequence of kernels meant to be executed as a unique stream of operations.
   */
  const std::vector<kernel::Kernel *> _kernels;
};

} // namespace ascend

} // namespace backend

} // namespace HiCR
