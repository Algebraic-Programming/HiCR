#pragma once

#include "common.hpp"
#include <hicr/L0/topology.hpp>
#include <hicr/L1/instanceManager.hpp>
#include <backends/host/hwloc/L1/memoryManager.hpp>

void coordinatorFc(HiCR::L1::InstanceManager &instanceManager, size_t requestedInstances)
{
  // Querying instance list
  auto &instances = instanceManager.getInstances();

  // Getting the pointer to our own (coordinator) instance
  auto coordinator = instanceManager.getCurrentInstance();

  // Check whether we have already the requested number of instances
  if (requestedInstances != instances.size())
  {
   // If not, requesting workers to abort and printing error message 
   for (const auto &instance : instances)
    if (instance->getId() != coordinator->getId())
      instanceManager.execute(*instance, PROCESSING_UNIT_ID, ABORT_EXECUTION_UNIT_ID);

   fprintf(stderr, "Error: number of instances obtained (%lu) differs from those requested (%lu)\n", instances.size(), requestedInstances);
   HiCR::finalize();
   exit(-1);
  } 

  // Printing instance information and invoking a simple RPC if its not ourselves
  for (const auto &instance : instances)
    if (instance->getId() != coordinator->getId())
      instanceManager.execute(*instance, PROCESSING_UNIT_ID, TEST_EXECUTION_UNIT_ID);

  // Getting return values from the RPCs containing each of the worker's topology
  for (const auto &instance : instances) if (instance != coordinator)
  {
    // Getting return value as a memory slot
    auto returnValue = instanceManager.getReturnValue(*instance);

    // Printing return value
    printf("[Coordinator] Received from instance %lu: '%s'\n", instance->getId(), (const char*)returnValue->getPointer());
  }
}
