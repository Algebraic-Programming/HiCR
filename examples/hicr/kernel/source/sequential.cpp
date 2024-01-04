#include <backends/sequential/L1/computeManager.hpp>
#include <backends/sequential/L1/topologyManager.hpp>
#include <stdio.h>

int main(int argc, char **argv)
{
  // Initializing Sequential backend's topology manager
  HiCR::backend::sequential::L1::TopologyManager dm;

  // Asking backend to check the available devices
  dm.queryDevices();

  // Getting first device found
  auto d = *dm.getDevices().begin();

  // Initializing sequential backend
  HiCR::backend::sequential::L1::ComputeManager computeManager;

  auto fcLambda = []() { printf("Hello, World!\n"); };

  // Creating execution unit
  auto executionUnit = computeManager.createExecutionUnit(fcLambda);

  // Getting compute resources
  auto computeResources = d->getComputeResourceList();

  // Asking the processing unit to create a new execution state from the given execution unit (stateless)
  auto executionState = computeManager.createExecutionState(executionUnit);

  // Selecting the first compute resource found
  auto firstComputeResource = *computeResources.begin();

  // Creating processing unit from the compute resource
  auto processingUnit = computeManager.createProcessingUnit(firstComputeResource);

  // Initializing processing unit
  processingUnit->initialize();

  // Running compute unit with the newly created execution state
  processingUnit->start(std::move(executionState));

  return 0;
}
