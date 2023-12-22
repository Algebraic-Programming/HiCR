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
#include <backends/ascend/L0/executionUnit.hpp>
#include <backends/ascend/L0/processingUnit.hpp>
#include <backends/ascend/kernel.hpp>
#include <hicr/L1/computeManager.hpp>
#include <unordered_map>

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
  ComputeManager() : HiCR::L1::ComputeManager() {};
  ~ComputeManager() = default;

  /**
   * Creates a new execution unit encapsulating a function.
   * This function is currently not supported and throws a runtime exception.
   *
   * \param[in] executionUnit function to be executed
   *
   * \return throws exception
   */
  __USED__ inline HiCR::L0::ExecutionUnit *createExecutionUnit(HiCR::L0::ExecutionUnit::function_t executionUnit) override
  {
    HICR_THROW_RUNTIME("Ascend backend currently does not support this API");
  }

  /**
   * Creates an execution unit given a stream/vector of \p kernelOperations to be executed on the device
   *
   * \param[in] kernelOperations the sequence of kernel operations to executed
   *
   * \return a pointer to the new execution unit
   */
  __USED__ inline HiCR::L0::ExecutionUnit *createExecutionUnit(const std::vector<ascend::Kernel *> &kernelOperations)
  {
    return new L0::ExecutionUnit(kernelOperations);
  }

  private:

  /**
   * Create a new processing unit for the specified \p resource (device)
   *
   * \param[in] resource the deviceId in which the processing unit is to be created
   *
   * \return a pointer to the new processing unit
   */
  __USED__ inline std::unique_ptr<HiCR::L0::ProcessingUnit> createProcessingUnitImpl(HiCR::L0::ComputeResource* resource) const override
  {
    return std::make_unique<L0::ProcessingUnit>(resource);
  }
};

} // namespace L1

} // namespace ascend

} // namespace backend

} // namespace HiCR
