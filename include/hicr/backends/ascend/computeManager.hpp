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
#include <hicr/backends/ascend/common.hpp>
#include <hicr/backends/ascend/executionUnit.hpp>
#include <hicr/backends/ascend/init.hpp>
#include <hicr/backends/ascend/kernel.hpp>
#include <hicr/backends/ascend/processingUnit.hpp>
#include <hicr/backends/computeManager.hpp>
#include <unordered_map>
namespace HiCR
{

namespace backend
{

namespace ascend
{

/**
 * Implementation of the HiCR ascend backend compute manager.
 *
 * It stores the processing units detected by Ascend Computing Language.
 */
class ComputeManager final : public backend::ComputeManager
{
  public:

  /**
   * Constructor for the Compute Manager class for the ascend backend
   *
   * \param[in] i ACL initializer
   *
   */
  ComputeManager(const Initializer &i) : backend::ComputeManager(), _deviceStatusMap(i.getContexts()){};

  ~ComputeManager() = default;

  /**
   * Creates a new execution unit encapsulating a function.
   * This function is currently not supported and throws a runtime exception.
   *
   * \param[in] executionUnit function to be executed
   *
   * \return throws exception
   */
  __USED__ inline HiCR::ExecutionUnit *createExecutionUnit(HiCR::ExecutionUnit::function_t executionUnit) override
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
  __USED__ inline HiCR::ExecutionUnit *createExecutionUnit(const std::vector<kernel::Kernel *> &kernelOperations)
  {
    return new ExecutionUnit(kernelOperations);
  }

  /**
   * Get the device id associated with the host system
   *
   * \param computeResources the collection of computeResources
   *
   * \return the id associated with the host system
   */
  __USED__ inline const computeResourceId_t getHostId(computeResourceList_t computeResources)
  {
    for (const auto c : computeResources)
      if (_deviceStatusMap.at(c).deviceType == deviceType_t::Host) return c;

    HICR_THROW_RUNTIME("No ID associated with the host system");
  }

  private:

  /**
   * Keep track of the device context for each computeResourceId/deviceId
   */
  const std::unordered_map<computeResourceId_t, ascendState_t> &_deviceStatusMap;

  /**
   * Internal implementaion of queryComputeResource routine.
   *
   * \return list of compute resources detected by ACL
   */
  __USED__ inline computeResourceList_t queryComputeResourcesImpl() override
  {
    // new compute resources list to return
    computeResourceList_t computeResourceList;

    // add as many computing resources as devices
    for (const auto [deviceId, _] : _deviceStatusMap) computeResourceList.insert(deviceId);

    return computeResourceList;
  }

  /**
   * Create a new processing unit for the specified \p resource (device)
   *
   * \param[in] resource the deviceId in which the processing unit is to be created
   *
   * \return a pointer to the new processing unit
   */
  __USED__ inline std::unique_ptr<HiCR::ProcessingUnit> createProcessingUnitImpl(computeResourceId_t resource) const override
  {
    if (_deviceStatusMap.at(resource).deviceType == deviceType_t::Host) HICR_THROW_RUNTIME("Ascend backend can not create a processing unit on the host.");
    return std::make_unique<ProcessingUnit>(resource);
  }
};

} // namespace ascend
} // namespace backend
} // namespace HiCR
