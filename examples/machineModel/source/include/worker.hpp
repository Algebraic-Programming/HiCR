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

// Creating Topology serialization function
void topologyFc (HiCR::L1::InstanceManager &instanceManager, std::shared_ptr<HiCR::L0::MemorySpace> bufferMemorySpace)
{
  // Fetching memory manager
  auto memoryManager = instanceManager.getMemoryManager();
  
  // Storage for the topology to send
  HiCR::L0::Topology workerTopology;

  // List of topology managers to query
  std::vector<HiCR::L1::TopologyManager*> topologyManagerList;

  // Now instantiating topology managers (which ones is determined by backend availability during compilation)
  #ifdef _HICR_USE_HWLOC_BACKEND_

  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Initializing HWLoc-based host (CPU) topology manager
  HiCR::backend::host::hwloc::L1::TopologyManager hwlocTopologyManager(&topology);

  // Adding topology manager to the list
  topologyManagerList.push_back(&hwlocTopologyManager);

  #endif // _HICR_USE_HWLOC_BACKEND_

  #ifdef _HICR_USE_ASCEND_BACKEND_

  // Initialize (Ascend's) ACL runtime
  aclError err = aclInit(NULL);
  if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Failed to initialize Ascend Computing Language. Error %d", err);

  // Initializing ascend topology manager
  HiCR::backend::ascend::L1::TopologyManager ascendTopologyManager;

  // Adding topology manager to the list
  topologyManagerList.push_back(&ascendTopologyManager);

  #endif // _HICR_USE_ASCEND_BACKEND_

  // For each topology manager detected
  for (const auto& tm : topologyManagerList)
  {
    // Getting the topology information from the topology manager
    const auto t = tm->queryTopology();

    // Merging its information to the worker topology object to send
    workerTopology.merge(t);
  } 

  // Serializing theworker topology and dumping it into a raw string message
  auto message = workerTopology.serialize().dump();

  // Registering memory slot at the first available memory space as source buffer to send the return value from
  auto sendBuffer = memoryManager->registerLocalMemorySlot(bufferMemorySpace, message.data(), message.size() + 1);

  // Registering return value
  instanceManager.submitReturnValue(sendBuffer);

  // Deregistering memory slot
  memoryManager->deregisterLocalMemorySlot(sendBuffer);
}

void workerFc(HiCR::L1::InstanceManager &instanceManager, std::shared_ptr<HiCR::L0::MemorySpace> bufferMemorySpace, std::shared_ptr<HiCR::L0::ComputeResource> computeResource)
{
  // Flag to indicate whether the worker should continue listening
  bool continueListening = true;

  // Creating task execution units
  auto taskAExecutionUnit = HiCR::backend::host::L1::ComputeManager::createExecutionUnit([&](){ taskFc("A", instanceManager, bufferMemorySpace); });
  auto taskBExecutionUnit = HiCR::backend::host::L1::ComputeManager::createExecutionUnit([&](){ taskFc("B", instanceManager, bufferMemorySpace); });
  auto taskCExecutionUnit = HiCR::backend::host::L1::ComputeManager::createExecutionUnit([&](){ taskFc("C", instanceManager, bufferMemorySpace); });

  // Creating topology reporting execution unit
  auto topologyExecutionUnit = HiCR::backend::host::L1::ComputeManager::createExecutionUnit([&](){ topologyFc(instanceManager, bufferMemorySpace); });

  // Creating finalization execution unit to end the worker's execution
  auto finalizationExecutionUnit = HiCR::backend::host::L1::ComputeManager::createExecutionUnit([&](){ continueListening = false; });

  // Creating processing unit from the compute resource
  auto processingUnit = instanceManager.getComputeManager()->createProcessingUnit(computeResource);

  // Assigning processing unit to the instance manager
  instanceManager.addProcessingUnit(PROCESSING_UNIT_ID, std::move(processingUnit));

  // Assigning execution units to the instance manager
  instanceManager.addExecutionUnit(FINALIZATION_RPC_ID, finalizationExecutionUnit);
  instanceManager.addExecutionUnit(TOPOLOGY_RPC_ID, topologyExecutionUnit);
  instanceManager.addExecutionUnit(TASK_A_RPC_ID, taskAExecutionUnit);
  instanceManager.addExecutionUnit(TASK_B_RPC_ID, taskBExecutionUnit);
  instanceManager.addExecutionUnit(TASK_C_RPC_ID, taskCExecutionUnit);

  // Adding listenable units (combines execution units with the processing unit in charge of executing them)
  instanceManager.addListenableUnit("Finalize", FINALIZATION_RPC_ID, PROCESSING_UNIT_ID);
  instanceManager.addListenableUnit("Get Topology", TOPOLOGY_RPC_ID, PROCESSING_UNIT_ID);
  instanceManager.addListenableUnit("Task A", TASK_A_RPC_ID, PROCESSING_UNIT_ID);
  instanceManager.addListenableUnit("Task B", TASK_B_RPC_ID, PROCESSING_UNIT_ID);
  instanceManager.addListenableUnit("Task C", TASK_C_RPC_ID, PROCESSING_UNIT_ID);
  
  // Listening for RPC requests
  while(continueListening == true) instanceManager.listen();
}
