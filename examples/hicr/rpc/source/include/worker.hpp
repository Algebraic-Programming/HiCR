#pragma once

#include <hicr/L1/instanceManager.hpp>
#include <hicr/backends/sequential/L1/computeManager.hpp>
#include "common.hpp"

void workerFc(HiCR::L1::InstanceManager& instanceManager)
{
 // Initializing sequential backend
 HiCR::backend::sequential::L1::ComputeManager computeManager;

 // Fetching memory manager
 auto memoryManager = instanceManager.getMemoryManager();

 // Getting current instance
 auto currentInstance = instanceManager.getCurrentInstance();

 // Creating worker function
 auto fcLambda = [currentInstance, memoryManager]()
 {
  // Creating simple message
  auto message = std::string("Hello, I am a worker! ");

  // Adding n characters to make the return values of variable length
  for (size_t i = 0; i < currentInstance->getId(); i++) message += std::string("*");

  // Registering memory slot at the first available memory space as source buffer to send the return value from
  auto sendBuffer = memoryManager->registerLocalMemorySlot(message.data(), message.size()+1);

  // Registering return value
  currentInstance->submitReturnValue(sendBuffer);

  // Deregistering memory slot
  memoryManager->deregisterLocalMemorySlot(sendBuffer);
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
