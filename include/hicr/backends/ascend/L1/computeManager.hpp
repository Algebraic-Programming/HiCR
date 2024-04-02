/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file computeManager.hpp
 * @brief This is a minimal backend for compute management of Ascend devices
 * @author S. M. Martin and L. Terracciano
 * @date 30/10/2023
 */

#pragma once

#include <acl/acl.h>
#include <unordered_map>
#include <hicr/backends/ascend/L0/executionUnit.hpp>
#include <hicr/backends/ascend/L0/processingUnit.hpp>
#include <hicr/backends/ascend/kernel.hpp>
#include <hicr/core/L1/computeManager.hpp>

namespace HiCR
{

namespace backend
{

namespace ascend
{

namespace L1
{

/**
 * Implementation of the HiCR ascend backend compute manager.
 *
 * It stores the processing units detected by Ascend Computing Language.
 */
class ComputeManager final : public HiCR::L1::ComputeManager
{
  public:

  /**
   * Constructor for the Compute Manager class for the ascend backend
   */
  ComputeManager()
    : HiCR::L1::ComputeManager(){};
  ~ComputeManager() = default;

  /**
   * Creates an execution unit given a stream/vector of \p kernelOperations to be executed on the device
   *
   * \param[in] kernelOperations the sequence of kernel operations to executed
   *
   * \return a pointer to the new execution unit
   */
  __INLINE__ std::shared_ptr<HiCR::L0::ExecutionUnit> createExecutionUnit(const std::vector<ascend::Kernel *> &kernelOperations)
  {
    return std::make_shared<L0::ExecutionUnit>(kernelOperations);
  }

  /**
   * Creates an execution state using the device context information and the exection unit to run on the ascend
   *
   * \param executionUnit execution Unit to launch on the ascend
   *
   * \return return a unique pointer to the newly created Execution State
   */
  __INLINE__ std::unique_ptr<HiCR::L0::ExecutionState> createExecutionState(std::shared_ptr<HiCR::L0::ExecutionUnit> executionUnit) const override
  {
    return std::make_unique<ascend::L0::ExecutionState>(executionUnit);
  }

  /**
   * Create a new processing unit for the specified \p resource (device)
   *
   * \param[in] resource the deviceId in which the processing unit is to be created
   *
   * \return a pointer to the new processing unit
   */
  __INLINE__ std::unique_ptr<HiCR::L0::ProcessingUnit> createProcessingUnit(std::shared_ptr<HiCR::L0::ComputeResource> resource) const override
  {
    return std::make_unique<L0::ProcessingUnit>(resource);
  }
};

} // namespace L1

} // namespace ascend

} // namespace backend

} // namespace HiCR
