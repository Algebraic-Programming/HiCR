/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file executionState.hpp
 * @brief This file implements the execution state class for the ascend backend
 * @author S. M. Martin & L. Terracciano
 * @date 9/10/2023
 */

#pragma once

#include <acl/acl.h>
#include <hicr/backends/ascend/executionUnit.hpp>
#include <hicr/common/exceptions.hpp>
#include <hicr/executionState.hpp>

namespace HiCR
{

namespace backend
{

namespace ascend
{

/**
 * This class represents the execution state of a kernel for the ascend backend.
 * Since kernels are not preemptible, it does not offer suspend/resume functionality.
 */
class ExecutionState final : public HiCR::ExecutionState
{
  public:

  __USED__ inline const std::string getKernelDirectory() const
  {
    return _kernelDir;
  }
  __USED__ inline const std::string getOpType() const
  {
    return _opType;
  }
  __USED__ inline size_t getInputSize() const
  {
    return _inputSize;
  }
  __USED__ inline size_t getOutputSize() const
  {
    return _outputSize;
  }
  __USED__ inline const aclTensorDesc *const *getInputTensorDescriptors() const
  {
    return _inputTensorDescriptors;
  }
  __USED__ inline const aclTensorDesc *const *getOutputTensorDescriptors() const
  {
    return _outputTensorDescriptors;
  }
  __USED__ inline const aclDataBuffer *const *getInputDataBuffers() const
  {
    return _inputDataBuffers;
  }
  __USED__ inline const aclDataBuffer *const *getOutputDataBuffers() const
  {
    return _outputDataBuffers;
  }
  __USED__ inline const aclopAttr *getKernelAttributes() const
  {
    return _kernelAttributes;
  }
  __USED__ inline const void *getModelPtr() const
  {
    return _modelPtr;
  }
  __USED__ inline const size_t getModelSize() const
  {
    return _modelSize;
  }

  protected:

  __USED__ inline void initializeImpl(const HiCR::ExecutionUnit *executionUnit) override
  {
    // Getting up-casted pointer for the execution unit
    auto e = dynamic_cast<const ExecutionUnit *>(executionUnit);

    // Checking whether the execution unit passed is compatible with this backend
    if (e == NULL) HICR_THROW_LOGIC("The passed execution of type '%s' is not supported by this backend\n", executionUnit->getType());

    _opType = e->getOpType();
    _kernelDir = e->getKernelDirectory();
    _inputSize = e->getInputSize();
    _outputSize = e->getOutputSize();
    _inputTensorDescriptors = e->getInputTensorDescriptors().data();
    _outputTensorDescriptors = e->getOutputTensorDescriptors().data();
    _inputDataBuffers = e->getInputDataBuffers().data();
    _outputDataBuffers = e->getOutputDataBuffers().data();
    _kernelAttributes = e->getKernelAttributes();
    _modelPtr = e->getModelPtr();
    _modelSize = e->getModelSize();
  }

  __USED__ inline void resumeImpl() override
  {
    HICR_THROW_RUNTIME("Resume functionality not supported by ascend backend");
  }

  __USED__ inline void suspendImpl()
  {
    HICR_THROW_RUNTIME("Suspend functionality not supported by ascend backend");
  }

  __USED__ inline bool checkFinalizationImpl() override
  {
    HICR_THROW_RUNTIME("Not implemented yet");
  }

  private:

  std::string _opType;
  std::string _kernelDir;
  size_t _inputSize;
  size_t _outputSize;
  const aclTensorDesc *const *_inputTensorDescriptors;
  const aclTensorDesc *const *_outputTensorDescriptors;
  const aclDataBuffer *const *_inputDataBuffers;
  const aclDataBuffer *const *_outputDataBuffers;
  const aclopAttr *_kernelAttributes;
  const void *_modelPtr;
  size_t _modelSize;
};

} // end namespace ascend

} // namespace backend

} // namespace HiCR
