/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file function.hpp
 * @brief Implements the compute unit (kernel) class for the Ascend backend.
 * @author S. M. Martin
 * @date 11/10/2023
 */

#pragma once

#include <functional>
#include <hicr/executionUnit.hpp>

namespace HiCR
{

namespace backend
{

namespace ascend
{

class Kernel final : public ExecutionUnit
{
  public:

  /**
   * Constructor for the Process class
   *
   * \param process An id for the process (should be zero)
   */
  Kernel(const std::string &kernelFilePath) : ExecutionUnit(), _kernelFilePath(kernelFilePath){};
  Kernel() = delete;
  ~Kernel() = default;

  __USED__ inline std::string identifyExecutionUnitType() override { return "Ascend Kernel"; }
  __USED__ inline const std::string &getKernel() { return _kernelFilePath; }

  private:

  std::string _kernelFilePath;
};

} // end namespace ascend

} // namespace backend

} // namespace HiCR
