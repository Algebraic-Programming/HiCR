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
 * @brief This file implements the computation kernel class for the acl backend
 * @author S. M. Martin & L. Terracciano
 * @date 8/11/2023
 */

#pragma once

#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <vector>
#include <acl/acl.h>
#include <hicr/core/localMemorySlot.hpp>
#include <hicr/core/exceptions.hpp>
#include <hicr/backends/acl/localMemorySlot.hpp>
#include <hicr/backends/acl/kernel.hpp>

namespace HiCR::backend::acl
{
/**
 * This class represents a replicable Computation Kernel for the acl backend.
 * A Computation Kernel enables the kernel execution in the HiCR runtime, and in particular enables
 * the concatenation of kernel execution and memcpy operations in a common stream of operations.
 */
class ComputationKernel final : public Kernel
{
  public:

  /**
   * Keep track of input and output tensor-specific data for executing acl kernel
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
   * Constructor for the Computation Kernel unit class of the acl backend.
   * This will not perform any model loading so this aspect should be handled manually (e.g., with aclopSetModelDir())
   *
   * \param kernelName name of the kernel
   * \param inputs kernel input tensor data descriptors
   * \param outputs kernel output tensor data descriptors
   * \param kernelAttrs kernel attributes
   */
  ComputationKernel(const char *kernelName, const std::vector<tensorData_t> &inputs, const std::vector<tensorData_t> &outputs, const aclopAttr *kernelAttrs)
    : Kernel(),
      _kernelName(kernelName),
      _kernelAttrs(kernelAttrs)
  {
    // populate internal data structure with input and output tensor data
    initializeDataBuffersAndDescriptors(inputs, _inputTensorDescriptors, _inputDataBuffers);
    initializeDataBuffersAndDescriptors(outputs, _outputTensorDescriptors, _outputDataBuffers);
  };

  /**
   * Constructor for the Computation Kernel unit class of the acl backend.
   * This will load an operator binary file located at the provided path with aclopLoad()
   *
   * \param kernelPath path the the kernel .om file
   * \param kernelName name of the kernel
   * \param inputs kernel input tensor data descriptors
   * \param outputs kernel output tensor data descriptors
   * \param kernelAttrs kernel attributes
   */
  ComputationKernel(const char *kernelPath, const char *kernelName, const std::vector<tensorData_t> &inputs, const std::vector<tensorData_t> &outputs, const aclopAttr *kernelAttrs)
    : ComputationKernel(kernelName, inputs, outputs, kernelAttrs)
  {
    // load kernel in memory
    loadKernel(std::string(kernelPath));
  };

  ComputationKernel()  = delete;
  ~ComputationKernel() = default;

  /**
   * Creates the acl-specific Tensor data to be used as input/output parameter to acl kernels
   *
   * \param memorySlot The memory slot to be used as input/output
   * \param tensorDescriptor acl-specific metadata about the passed memory slot
   * \return The new acl tensor data object
   */
  static tensorData_t createTensorData(const std::shared_ptr<HiCR::LocalMemorySlot> &memorySlot, aclTensorDesc *tensorDescriptor)
  {
    // Using up-casting to determine device types
    auto aclSlot = dynamic_pointer_cast<acl::LocalMemorySlot>(memorySlot);

    // Checking whether the memory slot passed is compatible with this backend
    if (aclSlot == NULL) HICR_THROW_LOGIC("Attempting to create acl tensor data with a memory slot that is not supported by this backend\n");

    // Creating and returning new tensor
    return acl::ComputationKernel::tensorData_t{.dataBuffer = aclSlot->getDataBuffer(), .tensorDescriptor = tensorDescriptor};
  }

  /**
   * Start the kernel using the given ACL \p stream
   *
   * \param stream the ACL stream on which the kernel is to be executed
   */
  __INLINE__ void start(const aclrtStream stream) override
  {
    // start the kernel
    aclError err = aclopExecuteV2(_kernelName.c_str(),
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
  std::string _kernelPtr;
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
  __INLINE__ void initializeDataBuffersAndDescriptors(const std::vector<tensorData_t>     tensors,
                                                      std::vector<const aclTensorDesc *> &descriptors,
                                                      std::vector<const aclDataBuffer *> &dataBuffers)
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
  __INLINE__ void loadKernel(const std::string &kernelPath)
  {
    // get size of file to know how much memory to allocate
    std::uintmax_t filesize = std::filesystem::file_size(kernelPath);
    _kernelSize             = filesize;

    // allocate buffer to hold file
    _kernelPtr.resize(_kernelSize);

    // read file
    std::ifstream fin(kernelPath, std::ios::binary);
    fin.read((char *)_kernelPtr.data(), _kernelSize);
    if (!fin) HICR_THROW_RUNTIME("Error reading file could only read %d bytes", fin.gcount());

    fin.close();

    // register the operator in the ACL runtime
    aclError err = aclopLoad(_kernelPtr.data(), _kernelSize);

    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Failed to load kernel into memory. Error %d", err);
  }
};

} // namespace HiCR::backend::acl
