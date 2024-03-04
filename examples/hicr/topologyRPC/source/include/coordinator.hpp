#pragma once

#include "common.hpp"
#include <hicr/L0/topology.hpp>
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
      instanceManager.launchRPC(*instance, TOPOLOGY_RPC_NAME);

  // Getting return values from the RPCs containing each of the worker's topology
  for (const auto &instance : instances)
    if (instance != coordinator)
    {
      // Getting return value as a memory slot
      auto returnValue = instanceManager.getReturnValue(*instance);

      // Receiving raw serialized topology information from the worker
      std::string serializedTopology = (char *)returnValue;

      // Parsing serialized raw topology into a json object
      auto topologyJson = nlohmann::json::parse(serializedTopology);

      // Freeing return value
      free(returnValue);

      // HiCR topology object to obtain
      HiCR::L0::Topology topology;

// Obtaining the topology from the serialized object
#ifdef _HICR_USE_HWLOC_BACKEND_
      topology.merge(HiCR::backend::host::hwloc::L1::TopologyManager::deserializeTopology(topologyJson));
#endif // _HICR_USE_HWLOC_BACKEND_

#ifdef _HICR_USE_ASCEND_BACKEND_
      topology.merge(HiCR::backend::ascend::L1::TopologyManager::deserializeTopology(topologyJson));
#endif // _HICR_USE_ASCEND_BACKEND_

      // Now summarizing the devices seen by this topology
      printf("* Worker %lu Topology:\n", instance->getId());
      for (const auto &d : topology.getDevices())
      {
        printf("  + '%s'\n", d->getType().c_str());
        printf("    Compute Resources: %lu %s(s)\n", d->getComputeResourceList().size(), (*d->getComputeResourceList().begin())->getType().c_str());
        for (const auto &m : d->getMemorySpaceList()) printf("    Memory Space:     '%s', %f Gb\n", m->getType().c_str(), (double)m->getSize() / (double)(1024ul * 1024ul * 1024ul));
      }
    }
}
