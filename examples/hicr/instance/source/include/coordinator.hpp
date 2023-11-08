#pragma once

#include <hicr/backends/instanceManager.hpp>
#include <include/common.hpp>

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
  printf("Worker state: %s\n", HiCR::Instance::getStateString(state).c_str());

  // If it is a worker instance, invoke an RPC
  if (instance != coordinator) instance->invoke(TEST_RPC_PROCESSING_UNIT_ID, TEST_RPC_EXECUTION_UNIT_ID);
 }
}



