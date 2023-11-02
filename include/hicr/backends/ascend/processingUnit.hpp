/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file processingUnit.hpp
 * @brief Implements the processing unit class for the ascend backend.
 * @author S. M. Martin & L. Terracciano
 * @date 11/9/2023
 */

#pragma once

#include <hicr/backends/ascend/executionUnit.hpp>
#include <hicr/common/exceptions.hpp>
#include <hicr/processingUnit.hpp>

namespace HiCR
{

namespace backend
{

namespace ascend
{

/**
 * Implementation of a processing unit (an device kernel) for the ascend backend
 */
class ProcessingUnit final : public HiCR::ProcessingUnit
{
  public:

  /**
   * Constructor for the Processing Unit (kernel) class
   *
   * \param process An id for the kernel
   */
  __USED__ inline ProcessingUnit(computeResourceId_t device, aclrtContext context) : HiCR::ProcessingUnit(device), _context(context){};

  private:

  const aclrtContext _context;

  /**
   * Variable to hold the execution state to run
   */
  std::unique_ptr<ExecutionState> _executionState;

  aclrtStream _stream;

  __USED__ inline void setDeviceContext() const
  {
    aclError err = aclrtSetCurrentContext(_context);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Error while setting the device context");
  }

  __USED__ inline void initializeImpl() override
  {
  }

  __USED__ inline void suspendImpl() override
  {
    HICR_THROW_RUNTIME("Resume functionality not supported by ascend backend");
  }

  __USED__ inline void resumeImpl() override
  {
    HICR_THROW_RUNTIME("Resume functionality not supported by ascend backend");
  }

  __USED__ inline void startImpl(std::unique_ptr<HiCR::ExecutionState> executionState) override
  {
    // Getting up-casted pointer for the execution unit
    std::unique_ptr<ExecutionState> e = std::unique_ptr<ExecutionState>(dynamic_cast<ExecutionState *>(executionState.release()));

    // Checking whether the execution unit passed is compatible with this backend
    if (e == NULL) HICR_THROW_LOGIC("The execution state is not supported by this backend\n");

    _executionState = std::move(e);

    auto opType = _executionState.get()->getOpType();
    auto inputSize = _executionState.get()->getInputSize();
    auto outputSize = _executionState.get()->getOutputSize();
    auto inputTensorDescriptors = _executionState.get()->getInputTensorDescriptors();
    auto outputTensorDescriptors = _executionState.get()->getOutputTensorDescriptors();
    auto inputDataBuffers = _executionState.get()->getInputDataBuffers();
    auto outputDataBuffers = _executionState.get()->getOutputDataBuffers();
    auto kernelAttributes = _executionState.get()->getKernelAttributes();
    auto kernelDir = _executionState.get()->getKernelDirectory();

    setDeviceContext();
    // TODO: Do not create here the stream
    aclError err = aclrtCreateStream(&_stream);

    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Failed to create a stream on the ascend");

    err = aclopSetModelDir(kernelDir.c_str());
    printf("look in dir %s\n", kernelDir.c_str());

    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Failed to set the directory %s where the model resides. Error %d", kernelDir.c_str(), err);

    err = aclopExecuteV2(opType.c_str(),
                         (int)inputSize,
                         (aclTensorDesc **)inputTensorDescriptors,
                         (aclDataBuffer **)inputDataBuffers,
                         (int)outputSize,
                         (aclTensorDesc **)outputTensorDescriptors,
                         (aclDataBuffer **)outputDataBuffers,
                         (aclopAttr *)kernelAttributes,
                         _stream);

    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Failed to run the kernel. Error %d", err);
    // TODO: move in await
    err = aclrtSynchronizeStream(_stream);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Failed to synchronize on the stream during kernel execution. Error %d", err);

    err = aclrtDestroyStream(_stream);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Failed to delete the stream after kernel execution. Error %d", err);
  }

  __USED__ inline void terminateImpl() override
  {
    HICR_THROW_RUNTIME("Not implemented yet");
  }

  __USED__ inline void awaitImpl() override
  {
    HICR_THROW_RUNTIME("Not implemented yet");
  }
};

} // namespace ascend

} // namespace backend

} // namespace HiCR
