/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file computeManager.hpp
 * @brief This is a minimal backend for compute management of Ascend devices
 * @author S. M. Martin and L. Terracciano
 * @date 10/12/2023
 */

#pragma once

#include <acl/acl.h>
#include <hicr/backends/ascend/executionState.hpp>
#include <hicr/backends/ascend/executionUnit.hpp>
#include <hicr/backends/ascend/processingUnit.hpp>
#include <hicr/backends/computeManager.hpp>

namespace HiCR
{

namespace backend
{

namespace ascend
{

/**
 * Implementation of the HWloc-based HiCR Shared Memory backend's compute manager.
 *
 * It detects and returns the processing units detected by the HWLoc library
 */
class ComputeManager final : public backend::ComputeManager
{
  public:

  /**
   * Constructor for the compute manager class for the shared memory backend
   *
   * \param[in] topology An HWloc topology object that can be used to query the available computational resources
   */
  ComputeManager() : backend::ComputeManager(){};

  ~ComputeManager()
  {
    // Destroy Ascend contexts
    for (const auto memSpaceData : _deviceStatusMap) (void)aclrtDestroyContext(memSpaceData.second.context);
  }

  __USED__ inline HiCR::ExecutionUnit *createExecutionUnit(HiCR::ExecutionUnit::function_t executionUnit) override
  {
    HICR_THROW_RUNTIME("Ascend backend currently does not support this API");
  }

  __USED__ inline HiCR::ExecutionUnit *createExecutionUnit(const char *kernelPath, const std::vector<ExecutionUnit::tensorData_t> &inputs, const std::vector<ExecutionUnit::tensorData_t> &outputs, const aclopAttr *kernelAttrs)
  {
    return new ExecutionUnit(kernelPath, inputs, outputs, kernelAttrs);
  }

  private:

  /**
   * Keeps track of how many devices are connected to the host
   */
  uint32_t deviceCount;

  struct ascendState_t
  {
    /**
     * remember the context associated to a device
     */
    aclrtContext context;
  };

  /**
   * Keep track of the context for each memorySpaceId/deviceId
   */
  std::map<computeResourceId_t, ascendState_t> _deviceStatusMap;

  /**
   * Set the device on which the operations needs to be executed
   *
   * \param[in] memorySpace the device identifier
   */
  __USED__ inline void selectDevice(const computeResourceId_t resource)
  {
    aclError err;

    // select the device context on which we should allocate the memory
    err = aclrtSetCurrentContext(_deviceStatusMap[resource].context);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not set ascend device %d. Error %d", resource, err);
  }

  __USED__ inline computeResourceList_t queryComputeResourcesImpl() override
  { // Clearing existing memory space map
    _deviceStatusMap.clear();

    // New memory space list to return
    computeResourceList_t computeResourceList;

    // Ask ACL for available devices
    aclError err;
    err = aclrtGetDeviceCount(&deviceCount);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not retrieve ascend device count. Error %d", err);

    aclrtContext deviceContext;

    // Add as many memory spaces as devices
    for (uint32_t deviceId = 0; deviceId < deviceCount; ++deviceId)
    {
      // Create the device context
      err = aclrtCreateContext(&deviceContext, deviceId);
      if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not create context in ascend device %d. Error %d", deviceId, err);

      // Select the device by setting the context
      err = aclrtSetCurrentContext(deviceContext);
      if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Can not create context in ascend device %d. Error %d", deviceId, err);

      // update the internal data structure
      _deviceStatusMap[deviceId] = ascendState_t{.context = deviceContext};
      computeResourceList.insert(deviceId);
    }

    // TODO: do we need to model the host?
    _deviceStatusMap[deviceCount] = ascendState_t{};
    computeResourceList.insert(deviceCount);

    return computeResourceList;
  }

  __USED__ inline ProcessingUnit *createProcessingUnitImpl(computeResourceId_t resource) const override
  {
    return new ProcessingUnit(resource, _deviceStatusMap.at(resource).context);
  }

  __USED__ inline std::unique_ptr<HiCR::ExecutionState> createExecutionStateImpl() override
  {
    return std::make_unique<ascend::ExecutionState>();
  }
};

} // namespace ascend
} // namespace backend
} // namespace HiCR
