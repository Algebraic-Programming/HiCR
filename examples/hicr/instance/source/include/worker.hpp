#pragma once

#include <hicr/backends/sequential/computeManager.hpp>
#include <hicr/backends/instanceManager.hpp>
#include <include/common.hpp>

void workerFc(HiCR::backend::InstanceManager& instanceManager)
{
 // Initializing sequential backend
 HiCR::backend::sequential::ComputeManager computeManager;

 auto fcLambda = []() { printf("Hello, World!\n");};

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

 // Getting current instance
 auto currentInstance = instanceManager.getCurrentInstance();

 // Assigning processing unit to the instance manager
 currentInstance->addProcessingUnit(TEST_RPC_PROCESSING_UNIT_ID, std::move(processingUnit));

 // Assigning processing unit to the instance manager
 currentInstance->addExecutionUnit(TEST_RPC_EXECUTION_UNIT_ID, executionUnit);

 // Listening for RPC requests
 currentInstance->listen();
}
