#pragma once

#include <hicr/backends/instanceManager.hpp>
#include "common.hpp"

void coordinatorFc(HiCR::backend::InstanceManager& instanceManager)
{
 // Querying instance list
 auto& instances = instanceManager.getInstances();

 // Getting the pointer to our own (coordinator) instance
 auto coordinator = instanceManager.getCurrentInstance();

 // Printing instance information and invoking a simple RPC if its not ourselves
 for (const auto& instance : instances)
 {
  // Getting instance state
  auto state = instance->getState();

  // Printing state
  printf("Worker state: %s\n", HiCR::L0::Instance::getStateString(state).c_str());

  // If it is a worker instance, execute an RPC
  if (instance != coordinator) instance->execute(TEST_RPC_PROCESSING_UNIT_ID, TEST_RPC_EXECUTION_UNIT_ID);
 }

 // Getting return values from the RPCs
 for (const auto& instance : instances)
 {
  // If it is a worker instance, then get return value size
  if (instance != coordinator)
  {
   // Getting return value as a memory slot
   auto returnValue = instance->getReturnValue();

   // Printing value
   printf("Received Return value: '%s'\n", (char*)returnValue->getPointer());
  }
 }
}



