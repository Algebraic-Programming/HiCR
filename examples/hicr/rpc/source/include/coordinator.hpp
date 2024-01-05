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

  // Getting return values from the RPCs
  for (auto &instance : instances)
  {
    // If it is a worker instance, then get return value size
    if (instance.get() != coordinator)
    {
      // Getting return value as a memory slot
      auto returnValue = instanceManager.getReturnValue(*instance);

      // Printing value
      printf("Received Return value: '%s'\n", (char *)returnValue->getPointer());
    }
  }
}
