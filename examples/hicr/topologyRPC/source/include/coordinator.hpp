#pragma once

#include "common.hpp"
#include <hicr/L1/instanceManager.hpp>

void coordinatorFc(HiCR::L1::InstanceManager &instanceManager)
{
  // Querying instance list
  auto &instances = instanceManager.getInstances();

  // Getting the pointer to our own (coordinator) instance
  auto coordinator = instanceManager.getCurrentInstance();

  // Printing instance information and invoking a simple RPC if its not ourselves
  for (const auto &instance : instances)
    if (instance->getId() != coordinator->getId())
      instanceManager.execute(*instance, TEST_RPC_PROCESSING_UNIT_ID, TEST_RPC_EXECUTION_UNIT_ID);

  // Getting return values from the RPCs containing each of the worker's topology
  for (const auto &instance : instances)
  {
    // If it is a worker instance, then get return value size
    if (instance != coordinator)
    {
      // Printing worker id
      printf("* Worker %lu Topology:\n", instance->getId());

      // Getting return value as a memory slot
      auto returnValue = instanceManager.getReturnValue(*instance);

      // Receiving raw serialized topology information from the worker
      std::string serializedTopology = (char *)returnValue->getPointer();

      // Parsing serialized raw topology into a json object
      auto topologyJson = nlohmann::json::parse(serializedTopology);
    
      // Iterate over the entries received
      for (const auto& entry : topologyJson["Topology Managers"])
      {
        // Getting topology manager type 
        const auto type = entry["Type"].get<std::string>();

        // Storage for the topology manager to deserialize
        std::shared_ptr<HiCR::L1::TopologyManager> tm = nullptr;
        
        // For each entry in the topology, serialize it into the corresponding topology manager instance

        #ifdef _HICR_USE_HWLOC_BACKEND_

        // Initializing HWLoc-based host (CPU) topology manager        
        if (type == "HWLoc") tm = std::make_shared<HiCR::backend::host::hwloc::L1::TopologyManager>(entry["Contents"]);

        #endif // _HICR_USE_HWLOC_BACKEND_

        #ifdef _HICR_USE_ASCEND_BACKEND_
        
        // Initializing ascend topology manager
        if (type == "Ascend")  tm = std::make_shared<HiCR::backend::ascend::L1::TopologyManager>(entry["Contents"]);

        #endif // _HICR_USE_ASCEND_BACKEND_

        // Check that we recognized the received type
        if (tm.get() == nullptr) { fprintf(stderr, "Error: Could not recognize topology manager of type: %s\n", type.c_str()); } 

        // Now summarizing the devices seen by this topology manager
        for (const auto& d : tm->getDevices())
        {
          printf("  + '%s'\n", d->getType().c_str());
          printf("    Compute Resources: %lu %s(s)\n", d->getComputeResourceList().size(), (*d->getComputeResourceList().begin())->getType().c_str());
          for (const auto& m : d->getMemorySpaceList())      printf("    Memory Space:     '%s', %f Gb\n", m->getType().c_str(), (double)m->getSize() / (double)(1024ul*1024ul*1024ul));
        }
      }
    }
  }
}
