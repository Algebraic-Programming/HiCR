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

#pragma once

#include <hicr/core/device.hpp>
#include <hicr/core/executionUnit.hpp>

/**
 * Execute the execution unit on the specified compute resource
 * 
 * @param[in] computeManager
 * @param[in] computeResource
 * @param[in] executionUnit
*/
void executeKernel(HiCR::ComputeManager &computeManager, std::shared_ptr<HiCR::ComputeResource> &computeResource, std::shared_ptr<HiCR::ExecutionUnit> &executionUnit)
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