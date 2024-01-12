#pragma once

#include "common.hpp"
#include <hicr/L0/computeResource.hpp>
#include <hicr/L0/memorySpace.hpp>
#include <hicr/L0/topology.hpp>
#include <hicr/L1/instanceManager.hpp>
#include <backends/host/pthreads/L1/computeManager.hpp>
#include <backends/host/hwloc/L1/memoryManager.hpp>
#include <backends/host/hwloc/L1/topologyManager.hpp>

// Worker task functions
void taskFc(const std::string& taskName, HiCR::L1::InstanceManager &instanceManager, std::shared_ptr<HiCR::L0::MemorySpace> bufferMemorySpace)
{
  // Getting memory manager
  auto mm = instanceManager.getMemoryManager();

  // Getting current instance
  auto currentInstance = instanceManager.getCurrentInstance();

  // Serializing theworker topology and dumping it into a raw string message
  auto message = std::string("Hello, I am worker ") + std::to_string(currentInstance->getId()) + std::string(" executing Task: ") + taskName; 

  // Registering memory slot at the first available memory space as source buffer to send the return value from
  auto sendBuffer = mm->registerLocalMemorySlot(bufferMemorySpace, message.data(), message.size() + 1);

  // Registering return value
  instanceManager.submitReturnValue(sendBuffer);

  // Deregistering memory slot
  mm->deregisterLocalMemorySlot(sendBuffer);
};

void workerFc(HiCR::L1::InstanceManager &instanceManager, std::shared_ptr<HiCR::L0::MemorySpace> bufferMemorySpace, std::shared_ptr<HiCR::L0::ComputeResource> computeResource)
{
  // Creating task execution units
  auto taskAExecutionUnit = HiCR::backend::host::L1::ComputeManager::createExecutionUnit([&](){ taskFc("A", instanceManager, bufferMemorySpace); });
  auto taskBExecutionUnit = HiCR::backend::host::L1::ComputeManager::createExecutionUnit([&](){ taskFc("B", instanceManager, bufferMemorySpace); });
  auto taskCExecutionUnit = HiCR::backend::host::L1::ComputeManager::createExecutionUnit([&](){ taskFc("C", instanceManager, bufferMemorySpace); });

  // Creating abort execution unit as empty function in case the application needs to abort its workers
  auto abortExecutionUnit = HiCR::backend::host::L1::ComputeManager::createExecutionUnit([](){});

  // Creating processing unit from the compute resource
  auto processingUnit = instanceManager.getComputeManager()->createProcessingUnit(computeResource);

  // Initialize processing unit
  processingUnit->initialize();

  // Assigning processing unit to the instance manager
  instanceManager.addProcessingUnit(PROCESSING_UNIT_ID, std::move(processingUnit));

  // Assigning execution units to the instance manager
  instanceManager.addExecutionUnit(ABORT_EXECUTION_UNIT_ID, abortExecutionUnit);
  instanceManager.addExecutionUnit(TASK_A_EXECUTION_UNIT_ID, taskAExecutionUnit);
  instanceManager.addExecutionUnit(TASK_B_EXECUTION_UNIT_ID, taskBExecutionUnit);
  instanceManager.addExecutionUnit(TASK_C_EXECUTION_UNIT_ID, taskCExecutionUnit);

  // Listening for RPC requests
  instanceManager.listen();
}
