/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file computationKernel.hpp
 * @brief This file implements the computation kernel class for the ascend backend
 * @author S. M. Martin & L. Terracciano
 * @date 8/11/2023
 */

#pragma once

#include <iostream>
#include <regex>
#include <vector>
#include <filesystem>
#include <fstream>
#include <acl/acl.h>
#include <hicr/common/exceptions.hpp>
#include <backends/ascend/kernel.hpp>

namespace HiCR
{

namespace backend
{

namespace ascend
{

/**
 * This class represents a replicable Computation Kernel for the ascend backend.
 * A Computation Kernel enables the kernel execution in the HiCR runtime, and in particular enables
 * the concatenation of kernel execution and memcpy operations in a common stream of operations.
 */
class ComputationKernel final : public Kernel
{
  public:

  /**
   * Keep track of input and output tensor specific data for executing ascend kernel
   */
  struct tensorData_t
  {
    /**
     * Represents data about the memory location
     */
    const aclDataBuffer *dataBuffer;
    /**
     * Represents the type of data the tensor contains
     */
    const aclTensorDesc *tensorDescriptor;
  };

  /**
   * Constructor for the Computation Kernel unit class of the ascend backend
   *
   * \param kernelPath path the the kernel .om file
   * \param kernelName name of the kernel
   * \param inputs kernel input tensor data descriptors
   * \param outputs kernel output tensor data descriptors
   * \param kernelAttrs kernel attributes
   */
  ComputationKernel(const char *kernelPath, const char *kernelName, const std::vector<tensorData_t> &inputs, const std::vector<tensorData_t> &outputs, const aclopAttr *kernelAttrs) : Kernel(),
                                                                                                                                                                                       _kernelName(kernelName),
                                                                                                                                                                                       _kernelAttrs(kernelAttrs)
  {
    // populate internal data structure with input and output tensor data
    initializeDataBuffersAndDescriptors(inputs, _inputTensorDescriptors, _inputDataBuffers);
    initializeDataBuffersAndDescriptors(outputs, _outputTensorDescriptors, _outputDataBuffers);
    // load kernel in memory
    loadKernel(std::string(kernelPath));
  };
  ComputationKernel() = delete;

  ~ComputationKernel() { delete _kernelPtr; }

  /**
   * Start the kernel using the given ACL \p stream
   *
   * \param stream the ACL stream on which the kernel is to be executed
   */
  __USED__ inline void start(const aclrtStream stream) override
  {
    // register the operator in the ACL runtime
    aclError err = aclopLoad(_kernelPtr, _kernelSize);

    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Failed to load kernel into memory. Error %d", err);

    // start the kernel
    err = aclopExecuteV2(_kernelName.c_str(),
                         (int)_inputTensorDescriptors.size(),
                         (aclTensorDesc **)_inputTensorDescriptors.data(),
                         (aclDataBuffer **)_inputDataBuffers.data(),
                         (int)_outputTensorDescriptors.size(),
                         (aclTensorDesc **)_outputTensorDescriptors.data(),
                         (aclDataBuffer **)_outputDataBuffers.data(),
                         (aclopAttr *)_kernelAttrs,
                         stream);

    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Failed to run the kernel. Error %d", err);
  }

  private:

  /**
   * The operator name
   */
  const std::string _kernelName;
  /**
   * ACL kernel attributes
   */
  const aclopAttr *_kernelAttrs;
  /**
   * Descriptors of the tensors passed as input to the kernel
   */
  std::vector<const aclTensorDesc *> _inputTensorDescriptors;
  /**
   * Descriptors of the tensors passed as output to the kernel
   */
  std::vector<const aclTensorDesc *> _outputTensorDescriptors;
  /**
   * Data buffers of the tensors passed as input to the kernel
   */
  std::vector<const aclDataBuffer *> _inputDataBuffers;
  /**
   * Data buffers of the tensors passed as input to the kernel
   */
  std::vector<const aclDataBuffer *> _outputDataBuffers;
  /**
   * Pointer to where the kernel resides in memory after reading it from .om file
   */
  const char *_kernelPtr;
  /**
   * Kernel size in memory
   */
  size_t _kernelSize;

  /**
   * Read the data from the \p tensors vector and populates the \a descriptors and \a databuffers vectors
   *
   * \param[in] tensors vector of tensors data
   * \param[out] descriptors vector of tensor descriptors
   * \param[out] dataBuffers vector of data buffers
   */
  __USED__ inline void initializeDataBuffersAndDescriptors(const std::vector<tensorData_t> tensors, std::vector<const aclTensorDesc *> &descriptors, std::vector<const aclDataBuffer *> &dataBuffers)
  {
    for (const auto &tensor : tensors)
    {
      dataBuffers.push_back(tensor.dataBuffer);
      descriptors.push_back(tensor.tensorDescriptor);
    }
  }

  /**
   * Read the kernel .om file located at \p kernelPath in memory
   *
   * \param[in] kernelPath path to the compiled kernel file
   */
  __USED__ inline void loadKernel(const std::string kernelPath)
  {
    // get size of file to know how much memory to allocate
    std::uintmax_t filesize = std::filesystem::file_size(kernelPath);
    _kernelSize = filesize;

    // allocate buffer to hold file
    _kernelPtr = new char[_kernelSize];

    // read file
    std::ifstream fin(kernelPath, std::ios::binary);
    fin.read((char *)_kernelPtr, _kernelSize);
    if (!fin) HICR_THROW_RUNTIME("Error reading file could only read %d bytes", fin.gcount());

    fin.close();
  }
};

} // namespace ascend

} // namespace backend

} // namespace HiCR
