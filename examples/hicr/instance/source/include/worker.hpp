#pragma once

#include <hicr/backends/sequential/computeManager.hpp>
#include <hicr/backends/instanceManager.hpp>
#include <include/common.hpp>

void workerFc(HiCR::backend::InstanceManager& instanceManager)
{
 // Initializing sequential backend
 HiCR::backend::sequential::ComputeManager computeManager;

 // Getting current instance
 auto currentInstance = instanceManager.getCurrentInstance();

 // Creating worker function
 auto fcLambda = [currentInstance]()
 {
  // Creating simple message
  auto message = std::string("Hello, I am a worker!");

  // Registering return value
  currentInstance->submitReturnValue((uint8_t*)message.data(), message.size()+1);
 };

 // Creating execution unit
 auto executionUnit = computeManager.createExecutionUnit(fcLambda);

 // Querying compute resources
 computeManager.queryComputeResources();

 // Getting compute resources
 auto computeResources = computeManager.getComputeResourceList();

 // Creating processing unit from the compute resource
 auto processingUnit = computeManager.createProcessingUnit(*computeResources.begin());

 // Initialize processing unit
 processingUnit->initialize();

 // Assigning processing unit to the instance manager
 currentInstance->addProcessingUnit(TEST_RPC_PROCESSING_UNIT_ID, std::move(processingUnit));

 // Assigning processing unit to the instance manager
 currentInstance->addExecutionUnit(TEST_RPC_EXECUTION_UNIT_ID, executionUnit);

 // Listening for RPC requests
 currentInstance->listen();
}
