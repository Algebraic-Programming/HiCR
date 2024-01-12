#pragma once

#include "common.hpp"
#include <hicr/L0/topology.hpp>
#include <hicr/L1/instanceManager.hpp>
#include <backends/host/hwloc/L1/memoryManager.hpp>

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
  for (const auto &instance : instances) if (instance != coordinator)
  {
    // Getting return value as a memory slot
    auto returnValue = instanceManager.getReturnValue(*instance);

    // Printing return value
    printf("[Coordinator] Received from instance %lu: '%s'\n", instance->getId(), (const char*)returnValue->getPointer());
  }
}
