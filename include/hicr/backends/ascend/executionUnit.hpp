/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file executionUnit.hpp
 * @brief This file implements the execution unit class for the ascend backend
 * @author S. M. Martin & L. Terracciano
 * @date 9/10/2023
 */

#pragma once

#include <acl/acl.h>
#include <hicr/executionUnit.hpp>
#include <iostream>
#include <regex>
#include <vector>

namespace HiCR
{

namespace backend
{

namespace ascend
{

/**
 * This class represents a replicable C++ executable function for the sequential (and shared memory) backends.
 */
class ExecutionUnit final : public HiCR::ExecutionUnit
{
  public:

  struct DataIO
  {
    aclFormat format;
    aclDataType DataType;
    std::vector<int> dimensions;
  };

  /**
   * Constructor for the execution unit class of the ascend backend
   *
   * \param kernelPath Path to the kernel .om file
   * \param inputs Kernel input descriptors
   * \param outputs Kernel output descriptors
   */
  ExecutionUnit(const char *kernelPath, const std::vector<DataIO> inputs, std::vector<DataIO> outputs) : HiCR::ExecutionUnit(), _kernelPath(kernelPath), _opType(findOpType(kernelPath)), _inputs(inputs), _outputs(outputs){};
  ExecutionUnit() = delete;

  /**
   * Default destructor
   */
  ~ExecutionUnit() = default;

  __USED__ inline std::string getType() const override { return "Ascend Kernel"; }

  __USED__ inline const char *getKernelPath() { return _kernelPath; }
  __USED__ inline const char *getOpType() { return _opType; }
  __USED__ inline const std::vector<DataIO> getInputs() { return _inputs; }
  __USED__ inline const std::vector<DataIO> getOutputs() { return _outputs; }

  private:

  const char *_kernelPath;
  const char *_opType;
  const std::vector<DataIO> _inputs;
  const std::vector<DataIO> _outputs;

  static const char *findOpType(const char *operatorName)
  {
    std::string opStr = std::string(operatorName);
    std::istringstream stream(opStr);
    std::string token;
    char delimiter = '_';

    const auto isAlphaPattern = std::regex(R"(^[a-zA-Z]+$)");
    while (std::getline(stream, token, delimiter))
    {
      // Check if the string matches the pattern
      if (std::regex_match(token, isAlphaPattern)) return token.c_str();
    }

    HICR_THROW_RUNTIME("Unable to get the prefix from kernel path %s", operatorName);
  }
};

} // namespace ascend

} // namespace backend

} // namespace HiCR
