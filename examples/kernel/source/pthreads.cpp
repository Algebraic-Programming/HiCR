#include <hicr/backends/pthreads/L1/computeManager.hpp>

#include "../common.hpp"

int main(int argc, char **argv)
{
  // get the first compute resource of this hardware
  auto firstComputeResource = getFirstComputeResource();

  // Initializing the compute manager
  HiCR::backend::pthreads::L1::ComputeManager computeManager;

  // assign a value to pass to the function
  int x = 42;

  // Creating execution unit
  auto executionUnit = computeManager.createExecutionUnit([](void *arg) { printf("Hello, World! I have the value: %d\n", *((int *)arg)); });

  // Asking the processing unit to create a new execution state from the given execution unit (stateless)
  auto executionState = computeManager.createExecutionState(executionUnit, &x);

  // Creating processing unit from the compute resource
  auto processingUnit = computeManager.createProcessingUnit(firstComputeResource);

  // Initializing processing unit
  computeManager.initialize(processingUnit);

  // Running compute unit with the newly created execution state
  computeManager.start(processingUnit, executionState);

  // Waiting for thread to finish
  computeManager.await(processingUnit);

  return 0;
}
