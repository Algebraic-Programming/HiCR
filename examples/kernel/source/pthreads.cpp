#include <cstdio>
#include <hicr/backends/host/pthreads/L1/computeManager.hpp>
#include <hicr/backends/host/hwloc/L1/topologyManager.hpp>

int main(int argc, char **argv)
{
  // Creating HWloc topology object
  hwloc_topology_t hwlocTopology;

  // Reserving memory for hwloc
  hwloc_topology_init(&hwlocTopology);

  // Instantiating HWLoc-based host (CPU) topology manager
  HiCR::backend::host::hwloc::L1::TopologyManager topologyManager(&hwlocTopology);

  // Asking backend to check the available devices
  auto topology = topologyManager.queryTopology();

  // Getting first device found in the topology
  auto device = *topology.getDevices().begin();

  // Getting compute resources
  auto computeResources = device->getComputeResourceList();

  // Selecting the first compute resource found
  auto firstComputeResource = *computeResources.begin();

  // Initializing Pthread-based host (CPU) compute manager
  HiCR::backend::host::pthreads::L1::ComputeManager computeManager;

  // Creating execution unit
  auto executionUnit = computeManager.createExecutionUnit([](void *arg) { printf("Hello, World!\n"); });

  // Asking the processing unit to create a new execution state from the given execution unit (stateless)
  auto executionState = computeManager.createExecutionState(executionUnit);

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
