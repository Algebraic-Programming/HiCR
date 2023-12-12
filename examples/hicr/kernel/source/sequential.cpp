#include <backends/sequential/L1/computeManager.hpp>
#include <stdio.h>

int main(int argc, char **argv)
{
  // Initializing sequential backend
  HiCR::backend::sequential::L1::ComputeManager computeManager;

  auto fcLambda = []() { printf("Hello, World!\n"); };

  // Creating execution unit
  auto executionUnit = computeManager.createExecutionUnit(fcLambda);

  // Querying compute resources
  computeManager.queryComputeResources();

  // Getting compute resources
  auto computeResources = computeManager.getComputeResourceList();

  // Creating processing unit from the compute resource
  auto processingUnit = computeManager.createProcessingUnit(*computeResources.begin());

  // Initializing processing unit
  processingUnit->initialize();

  // Asking the processing unit to create a new execution state from the given execution unit (stateless)
  auto executionState = processingUnit->createExecutionState(executionUnit);

  // Running compute unit with the newly created execution state
  processingUnit->start(std::move(executionState));

  return 0;
}
