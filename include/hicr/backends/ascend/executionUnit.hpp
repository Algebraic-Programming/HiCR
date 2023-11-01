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
#include <hicr/backends/ascend/memorySlot.hpp>
#include <hicr/common/exceptions.hpp>
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
 * This class represents a replicable C++ executable function for the ascend backend.
 */
class ExecutionUnit final : public HiCR::ExecutionUnit
{
  public:

  struct tensorData_t
  {
    const MemorySlot *memorySlot;
    // TODO: extract in different class
    const std::vector<int64_t> dimensions;
    const aclDataType DataType;
    const aclFormat format;
  };

  /**
   * Constructor for the execution unit class of the ascend backend
   *
   * \param kernelPath Path to the kernel .om file
   * \param inputs Kernel input descriptors
   * \param outputs Kernel output descriptors
   * \param context The Ascend device context
   * \param kernelAttrs Kernel attributes
   */
  ExecutionUnit(const char *kernelPath, const std::vector<tensorData_t> &inputs, const std::vector<tensorData_t> &outputs, const aclopAttr *kernelAttrs) : HiCR::ExecutionUnit(),
                                                                                                                                                           _kernelPath(std::string(kernelPath)),
                                                                                                                                                           _opType(findOpType(_kernelPath)),
                                                                                                                                                           _kernelDir(findKernelDir(_kernelPath)),
                                                                                                                                                           _inputs(inputs),
                                                                                                                                                           _outputs(outputs),
                                                                                                                                                           _kernelAttrs(kernelAttrs)
  {
    initializeDataBuffersAndDescriptors(_inputs, _inputTensorDescriptors, _inputDataBuffers);
    initializeDataBuffersAndDescriptors(_outputs, _outputTensorDescriptors, _outputDataBuffers);
  };
  ExecutionUnit() = delete;

  /**
   * Default destructor
   */
  ~ExecutionUnit() = default;

  __USED__ inline std::string getType() const override { return "Ascend Kernel"; }

  __USED__ inline const std::string getKernelPath() const { return _kernelPath; }
  __USED__ inline const std::string getOpType() const { return _opType; }
  __USED__ inline const std::string getKernelDirectory() const { return _kernelDir; }
  __USED__ inline const aclopAttr *getKernelAttributes() const { return _kernelAttrs; }
  __USED__ inline const size_t getInputSize() const { return _inputs.size(); }
  __USED__ inline const size_t getOutputSize() const { return _outputs.size(); }

  __USED__ inline const std::vector<const aclTensorDesc *> &getInputTensorDescriptors() const { return _inputTensorDescriptors; }
  __USED__ inline const std::vector<const aclTensorDesc *> &getOutputTensorDescriptors() const { return _outputTensorDescriptors; }
  __USED__ inline const std::vector<const aclDataBuffer *> &getInputDataBuffers() const { return _inputDataBuffers; }
  __USED__ inline const std::vector<const aclDataBuffer *> &getOutputDataBuffers() const { return _outputDataBuffers; }

  private:

  const std::string _kernelPath;
  const std::string _opType;
  const std::string _kernelDir;
  const std::vector<tensorData_t> _inputs;
  const std::vector<tensorData_t> _outputs;
  const aclopAttr *_kernelAttrs;
  std::vector<const aclTensorDesc *> _inputTensorDescriptors;
  std::vector<const aclTensorDesc *> _outputTensorDescriptors;
  std::vector<const aclDataBuffer *> _inputDataBuffers;
  std::vector<const aclDataBuffer *> _outputDataBuffers;

  void initializeDataBuffersAndDescriptors(const std::vector<tensorData_t> tensorData, std::vector<const aclTensorDesc *> &tensorDescriptors, std::vector<const aclDataBuffer *> &dataBuffers)
  {
    for (size_t i = 0; i < tensorData.size(); i++)
    {
      const auto data = tensorData[i];
      dataBuffers.push_back(data.memorySlot->getDataBuffer());

      // TODO: pass directly the tensor descriptors?
      auto tensorDesc = aclCreateTensorDesc(data.DataType, data.dimensions.size(), data.dimensions.data(), data.format);
      if (tensorDesc == NULL) HICR_THROW_RUNTIME("Invalid call while creating descriptors for data tensors on Ascend Execution Unit");
      tensorDescriptors.push_back(tensorDesc);
    }
  }

  static std::string findOpType(const std::string kernelPath)
  {
    const auto operatorName = kernelPath.substr(kernelPath.rfind('/') + 1);
    auto stream = std::istringstream(operatorName);
    std::string token;
    char delimiter = '_';

    const auto isAlphaPattern = std::regex(R"(^[a-zA-Z]+$)");
    while (std::getline(stream, token, delimiter))
    {
      // Check if the string matches the pattern
      if (std::regex_match(token, isAlphaPattern)) return token;
    }

    HICR_THROW_RUNTIME("Unable to get the prefix from kernel path %s", operatorName);
  }

  static std::string findKernelDir(const std::string kernelPath)
  {
    const auto kernelDirectory = kernelPath.substr(0, kernelPath.rfind("/"));
    return kernelDirectory;
  }
};

} // namespace ascend

} // namespace backend

} // namespace HiCR
