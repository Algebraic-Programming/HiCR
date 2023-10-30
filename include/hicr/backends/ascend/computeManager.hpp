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
  ComputeManager(const char *config_path = NULL) : backend::ComputeManager()
  {
    aclError err;
    err = aclInit(config_path);

    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Failed to initialize Ascend Computing Language. Error %d", err);
  }

  ~ComputeManager(){
    (void)aclFinalize();
  }

  __USED__ inline ExecutionUnit *createExecutionUnit(HiCR::ExecutionUnit::function_t executionUnit) override
  {
    HICR_THROW_RUNTIME("Not implemented yet");
  }

  private:

  __USED__ inline computeResourceList_t queryComputeResourcesImpl() override
  {
    HICR_THROW_RUNTIME("Not implemented yet");
  }

  __USED__ inline ProcessingUnit *createProcessingUnitImpl(computeResourceId_t resource) const override
  {
    HICR_THROW_RUNTIME("Not implemented yet");
  }

  __USED__ inline std::unique_ptr<HiCR::ExecutionState> createExecutionStateImpl() override
  {
    HICR_THROW_RUNTIME("Not implemented yet");
  }
};

} // namespace sharedMemory
} // namespace backend
} // namespace HiCR
