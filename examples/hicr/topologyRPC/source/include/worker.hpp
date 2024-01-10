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

    // Storage for the topology managers to use to discover the system's hardware
    std::vector<std::pair<std::string, HiCR::L1::TopologyManager*>> topologyManagerList;

    // Now instantiating topology managers (which ones is determined by backend availability during compilation)
    #ifdef _HICR_USE_HWLOC_BACKEND_

    // Creating HWloc topology object
    hwloc_topology_t topology;

    // Reserving memory for hwloc
    hwloc_topology_init(&topology);

    // Initializing HWLoc-based host (CPU) topology manager
    HiCR::backend::host::hwloc::L1::TopologyManager hwlocTopologyManager(&topology);

    // Adding topology manager to the list
    topologyManagerList.push_back(std::make_pair("HWLoc", &hwlocTopologyManager));

    #endif // _HICR_USE_HWLOC_BACKEND_

    #ifdef _HICR_USE_ASCEND_BACKEND_

    // Initialize (Ascend's) ACL runtime
    aclError err = aclInit(NULL);
    if (err != ACL_SUCCESS) HICR_THROW_RUNTIME("Failed to initialize Ascend Computing Language. Error %d", err);

    // Initializing ascend topology manager
    HiCR::backend::ascend::L1::TopologyManager ascendTopologyManager;

    // Adding topology manager to the list
    topologyManagerList.push_back(std::make_pair("Ascend", &ascendTopologyManager));

    #endif // _HICR_USE_ASCEND_BACKEND_

    // Creating new serializable message (containing our system's topology) to transmit back to the coordinator
    nlohmann::json topologyJson;

    // Creating topology managers array
    topologyJson["Topology Managers"] = std::vector<nlohmann::json>();

    // For each topology manager detected
    for (const auto& m : topologyManagerList)
    {
      // Query the devices it can detect
      m.second->queryDevices();

      // Creating new entry for the serialized message
      nlohmann::json entry;
      
      // Writing type in the entry and serializing the topology manager contents
      entry["Type"] = m.first;
      entry["Contents"] = m.second->serialize();

      // Adding the entry to the list
      topologyJson["Topology Managers"] += entry;
    } 

    // Creating the serialized (human readable for simplcity, but it can be made binarized for compactness)
    auto message = topologyJson.dump(2);

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
