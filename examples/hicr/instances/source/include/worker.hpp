#pragma once

#include "common.hpp"
#include <hicr/L0/computeResource.hpp>
#include <hicr/L0/memorySpace.hpp>
#include <hicr/L0/topology.hpp>
#include <hicr/L1/instanceManager.hpp>
#include <backends/host/pthreads/L1/computeManager.hpp>
#include <backends/host/hwloc/L1/memoryManager.hpp>
#include <backends/host/hwloc/L1/topologyManager.hpp>

void workerFc(HiCR::L1::InstanceManager &instanceManager, std::shared_ptr<HiCR::L0::MemorySpace> bufferMemorySpace, std::shared_ptr<HiCR::L0::ComputeResource> computeResource)
{
  // Getting current instance
  auto currentInstance = instanceManager.getCurrentInstance();

  // Creating worker function
  auto fcLambda = [&]()
  {
    // Getting memory manager
    auto mm = instanceManager.getMemoryManager();
    
    // Serializing theworker topology and dumping it into a raw string message
    auto message = std::string("Hello, I am worker: ") + std::to_string(currentInstance->getId()); 

    // Registering memory slot at the first available memory space as source buffer to send the return value from
    auto sendBuffer = mm->registerLocalMemorySlot(bufferMemorySpace, message.data(), message.size() + 1);

    // Registering return value
    instanceManager.submitReturnValue(sendBuffer);

    // Deregistering memory slot
    mm->deregisterLocalMemorySlot(sendBuffer);
  };

  // Creating execution unit
  auto executionUnit = HiCR::backend::host::L1::ComputeManager::createExecutionUnit(fcLambda);

  // Creating processing unit from the compute resource
  auto processingUnit = instanceManager.getComputeManager()->createProcessingUnit(computeResource);

  // Initialize processing unit
  processingUnit->initialize();

  // Assigning processing unit to the instance manager
  instanceManager.addProcessingUnit(TEST_RPC_PROCESSING_UNIT_ID, std::move(processingUnit));

  // Assigning processing unit to the instance manager
  instanceManager.addExecutionUnit(TEST_RPC_EXECUTION_UNIT_ID, executionUnit);

  // Listening for RPC requests
  instanceManager.listen();
}
