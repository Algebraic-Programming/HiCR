#pragma once

#include <hicr/core/L0/device.hpp>
#include <hicr/core/L0/executionUnit.hpp>

/**
 * Execute the execution unit on the specified compute resource
 * 
 * @param[in] computeManager
 * @param[in] computeResource
 * @param[in] executionUnit
*/
void executeKernel(HiCR::L1::ComputeManager &computeManager, std::shared_ptr<HiCR::L0::ComputeResource> &computeResource, std::shared_ptr<HiCR::L0::ExecutionUnit> &executionUnit)
{
  // Create a processing unit on the desired compute resource
  auto processingUnit = computeManager.createProcessingUnit(computeResource);
  computeManager.initialize(processingUnit);

  // Create an execution state using the execution unit
  auto executionState = computeManager.createExecutionState(executionUnit);

  // Start the execution
  computeManager.start(processingUnit, executionState);

  // Start teminating the processing unit
  computeManager.terminate(processingUnit);

  // Wait for termination. After this point, the execution state has terminated the execution
  computeManager.await(processingUnit);
}