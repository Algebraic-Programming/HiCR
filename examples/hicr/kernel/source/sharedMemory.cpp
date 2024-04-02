#include <cstdio>
#include <hicr/backends/host/pthreads/L1/computeManager.hpp>
#include <hicr/backends/host/hwloc/L1/topologyManager.hpp>

int main(int argc, char **argv)
{
  // Creating HWloc topology object
  hwloc_topology_t topology;

  // Reserving memory for hwloc
  hwloc_topology_init(&topology);

  // Instantiating HWLoc-based host (CPU) topology manager
  HiCR::backend::host::hwloc::L1::TopologyManager tm(&topology);

  // Asking backend to check the available devices
  auto t = tm.queryTopology();

  // Getting first device found in the topology
  auto d = *t.getDevices().begin();

  // Initializing Pthread-based host (CPU) compute manager
  HiCR::backend::host::pthreads::L1::ComputeManager computeManager;

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

  // Waiting for thread to finish
  processingUnit->await();

  return 0;
}
