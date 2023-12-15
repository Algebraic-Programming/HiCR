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
#include <backends/ascend/common.hpp>
#include <backends/ascend/core.hpp>
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
   *
   * \param[in] i ACL initializer
   *
   */
  ComputeManager(const Core &i) : HiCR::L1::ComputeManager(), _deviceStatusMap(i.getContexts()){};

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
   * Keep track of the device context for each computeResourceId/deviceId
   */
  const std::unordered_map<deviceIdentifier_t, ascendState_t> &_deviceStatusMap;

  /**
   * Internal implementaion of queryComputeResource routine.
   *
   * \return list of compute resources detected by ACL
   */
  __USED__ inline computeResourceList_t queryComputeResourcesImpl() override
  {
    // new compute resources list to return
    computeResourceList_t computeResourceList;

    // Add as many memory spaces as devices
    for (const auto [deviceId, deviceState] : _deviceStatusMap)
    {
       // Getting device's type
      auto deviceType = deviceState.deviceType;

      // Creating new Memory Space object
      auto deviceComputeResource = new ascend::L0::ComputeResource(deviceType, deviceId);

      // Adding it to the list
      computeResourceList.insert(deviceComputeResource);
    } 

    return computeResourceList;
  }

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
