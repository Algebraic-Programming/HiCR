#pragma once

#include "common.hpp"
#include <hicr/L1/instanceManager.hpp>
#include <frontends/machineModel/machineModel.hpp>

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
  // Flag to indicate whether the worker should continue listening
  bool continueListening = true;

  // Creating machine model to handle the instance creation and task execution
  HiCR::MachineModel machineModel(instanceManager);

  // Creating task execution units
  auto taskAExecutionUnit = HiCR::backend::host::L1::ComputeManager::createExecutionUnit([&](){ taskFc("A", instanceManager, bufferMemorySpace); });
  auto taskBExecutionUnit = HiCR::backend::host::L1::ComputeManager::createExecutionUnit([&](){ taskFc("B", instanceManager, bufferMemorySpace); });
  auto taskCExecutionUnit = HiCR::backend::host::L1::ComputeManager::createExecutionUnit([&](){ taskFc("C", instanceManager, bufferMemorySpace); });

  // Creating finalization execution unit to end the worker's execution
  auto finalizationExecutionUnit = HiCR::backend::host::L1::ComputeManager::createExecutionUnit([&](){ continueListening = false; });

  // Creating processing unit from the compute resource
  auto processingUnit = instanceManager.getComputeManager()->createProcessingUnit(computeResource);

  // Assigning processing unit to the instance manager
  instanceManager.addProcessingUnit(std::move(processingUnit));

  // Assigning execution units to the instance manager
  instanceManager.addExecutionUnit(finalizationExecutionUnit, FINALIZATION_RPC_ID);
  instanceManager.addExecutionUnit(taskAExecutionUnit, TASK_A_RPC_ID);
  instanceManager.addExecutionUnit(taskBExecutionUnit, TASK_B_RPC_ID);
  instanceManager.addExecutionUnit(taskCExecutionUnit, TASK_C_RPC_ID);

  // Adding RPC targets, specifying a name and the execution unit to execute
  instanceManager.addRPCTarget("Finalize", FINALIZATION_RPC_ID);
  instanceManager.addRPCTarget("Task A", TASK_A_RPC_ID);
  instanceManager.addRPCTarget("Task B", TASK_B_RPC_ID);
  instanceManager.addRPCTarget("Task C", TASK_C_RPC_ID);
  
  // Listening for RPC requests
  while(continueListening == true) instanceManager.listen();
}
