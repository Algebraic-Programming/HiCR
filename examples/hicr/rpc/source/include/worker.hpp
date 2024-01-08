#pragma once

#include "common.hpp"
#include <hicr/L1/instanceManager.hpp>
#include <hicr/L1/memoryManager.hpp>
#include <hicr/L0/computeResource.hpp>
#include <hicr/L0/memorySpace.hpp>
#include <backends/host/L1/computeManager.hpp>

void workerFc(HiCR::L1::InstanceManager &instanceManager,
              HiCR::backend::host::L1::ComputeManager &computeManager,
              std::shared_ptr<HiCR::L0::MemorySpace> bufferMemorySpace,
              std::shared_ptr<HiCR::L0::ComputeResource> rpcExecutor)
{
  // Creating worker function
  auto fcLambda = [&]()
  {
    // Fetching memory manager
    auto memoryManager = instanceManager.getMemoryManager();
    
    // Getting current instance
    auto currentInstance = instanceManager.getCurrentInstance();

    // Creating simple message
    auto message = std::string("Hello, I am a worker! ");

    // Adding n characters to make the return values of variable length
    for (size_t i = 0; i < currentInstance->getId(); i++) message += std::string("*");

    // Registering memory slot at the first available memory space as source buffer to send the return value from
    auto sendBuffer = memoryManager->registerLocalMemorySlot(bufferMemorySpace, message.data(), message.size() + 1);

    // Registering return value
    instanceManager.submitReturnValue(sendBuffer);

    // Deregistering memory slot
    memoryManager->deregisterLocalMemorySlot(sendBuffer);
  };

  // Creating execution unit
  auto executionUnit = computeManager.createExecutionUnit(fcLambda);

  // Creating processing unit from the compute resource
  auto processingUnit = computeManager.createProcessingUnit(rpcExecutor);

  // Initialize processing unit
  processingUnit->initialize();

  // Assigning processing unit to the instance manager
  instanceManager.addProcessingUnit(TEST_RPC_PROCESSING_UNIT_ID, std::move(processingUnit));

  // Assigning processing unit to the instance manager
  instanceManager.addExecutionUnit(TEST_RPC_EXECUTION_UNIT_ID, executionUnit);

  // Listening for RPC requests
  instanceManager.listen();
}
