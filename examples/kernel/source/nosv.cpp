#include <nosv.h>
#include <hicr/backends/nosv/common.hpp>
#include <hicr/backends/nosv/L1/computeManager.hpp>

#include "../common.hpp"

int main(int argc, char **argv)
{
  // Initialize nosv
  check(nosv_init());

  // nosv task instance for the main thread
  nosv_task_t mainTask;

  // Attaching the main thread
  check(nosv_attach(&mainTask, NULL, NULL, NOSV_ATTACH_NONE));

  // get the first compute resource of this hardware
  auto firstComputeResource = getFirstComputeResource();

  // Initializing the compute manager
  HiCR::backend::nosv::L1::ComputeManager computeManager;

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

  // Detach main thread
  check(nosv_detach(NOSV_DETACH_NONE));

  // Shutdown nosv
  check(nosv_shutdown());

  return 0;
}
